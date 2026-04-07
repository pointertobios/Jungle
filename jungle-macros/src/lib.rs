use proc_macro::TokenStream;

use proc_macro_crate::{FoundCrate, crate_name};
use proc_macro2::Span;
use quote::{ToTokens, format_ident, quote};
use syn::spanned::Spanned;
use syn::{
    Attribute, Field, Fields, Ident, Item, ItemStruct, LitBool, Meta, Path, PathArguments, Result,
    Type, TypePath, parse::Parser, parse_macro_input, parse_quote,
};

struct ComponentArgs {
    storage: Path,
    exclusive: bool,
    noserde: bool,
}

struct SerdeField {
    ident: Ident,
    ty: Type,
    span: Span,
}

#[proc_macro_attribute]
pub fn component(attr: TokenStream, item: TokenStream) -> TokenStream {
    let args = parse_macro_input!(attr with parse_component_args);
    let input = parse_macro_input!(item as Item);

    let input = match input {
        Item::Struct(item_struct) => item_struct,
        other => {
            return syn::Error::new_spanned(
                other,
                "`#[component(...)]` can only be applied to structs",
            )
            .into_compile_error()
            .into();
        }
    };

    expand_component(args, input)
        .unwrap_or_else(syn::Error::into_compile_error)
        .into()
}

fn parse_component_args(input: syn::parse::ParseStream<'_>) -> Result<ComponentArgs> {
    let mut storage = None;
    let mut exclusive = None;
    let mut noserde = false;

    let parser = syn::meta::parser(|meta| {
        if meta.path.is_ident("noserde") {
            if noserde {
                return Err(meta.error("duplicate `noserde` argument; specify it only once"));
            }

            noserde = true;
            return Ok(());
        }

        if meta.path.is_ident("storage") {
            if storage.is_some() {
                return Err(meta.error("duplicate `storage` argument; specify it only once"));
            }

            let value = meta.value()?;
            storage = Some(value.parse::<Path>()?);
            return Ok(());
        }

        if meta.path.is_ident("exclusive") {
            if exclusive.is_some() {
                return Err(meta.error("duplicate `exclusive` argument; specify it only once"));
            }

            let value = meta.value()?;
            exclusive = Some(value.parse::<LitBool>()?.value());
            return Ok(());
        }

        let key = meta.path.to_token_stream().to_string();

        Err(meta.error(format!(
            "unknown component argument `{key}`; expected `noserde`, `storage = StorageType`, or `exclusive = true/false`"
        )))
    });

    parser.parse2(input.parse()?)?;

    let storage = storage.ok_or_else(|| {
        syn::Error::new(
            input.span(),
            "missing required `storage = StorageType` argument, for example `storage = SparseComponentStorage`",
        )
    })?;
    let exclusive = exclusive.ok_or_else(|| {
        syn::Error::new(
            input.span(),
            "missing required `exclusive = true/false` argument",
        )
    })?;

    Ok(ComponentArgs {
        storage,
        exclusive,
        noserde,
    })
}

fn expand_component(
    args: ComponentArgs,
    mut input: ItemStruct,
) -> Result<proc_macro2::TokenStream> {
    let entity_field = validate_component_struct(&input)?;
    validate_entity_field_type(entity_field)?;
    let serde_fields = collect_serde_fields(&mut input)?;

    if args.noserde && !serde_fields.is_empty() {
        return Err(syn::Error::new(
            serde_fields[0].span,
            "fields cannot use `#[serde]` when the component is marked `noserde`",
        ));
    }

    let ident = &input.ident;
    let storage = build_storage_type(&args.storage)?;
    let exclusive = args.exclusive;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    let jungle_core = resolve_jungle_core_path(input.span())?;
    let serde_impl = build_component_serde_impl(&args, &input, &serde_fields, &jungle_core);
    let jaml_impl = build_component_jaml_impl(&args, &serde_fields, &jungle_core);

    Ok(quote! {
        #input

        impl #impl_generics #jungle_core::ecs::component::Component for #ident #ty_generics #where_clause {
            type Storage = #storage;

            const EXCLUSIVE: bool = #exclusive;

            fn type_name() -> &'static str {
                ::std::stringify!(#ident)
            }

            fn downcast_unchecked<T: #jungle_core::ecs::component::Component>(&self) -> &T {
                self.as_any().downcast_ref::<T>().unwrap_or_else(|| {
                    panic!(
                        "component downcast failed: actual={} ({}) requested={} ({})",
                        <Self as #jungle_core::ecs::component::Component>::type_name(),
                        <Self as #jungle_core::ecs::component::Component>::id(),
                        T::type_name(),
                        T::id(),
                    )
                })
            }

            fn downcast_unchecked_mut<T: #jungle_core::ecs::component::Component>(&mut self) -> &mut T {
                self.as_any_mut().downcast_mut::<T>().unwrap_or_else(|| {
                    panic!(
                        "component downcast failed: actual={} ({}) requested={} ({})",
                        <Self as #jungle_core::ecs::component::Component>::type_name(),
                        <Self as #jungle_core::ecs::component::Component>::id(),
                        T::type_name(),
                        T::id(),
                    )
                })
            }

            #jaml_impl

            fn entity(&self) -> #jungle_core::ecs::entity::Entity {
                self.entity
            }
        }

        #serde_impl
    })
}

fn collect_serde_fields(input: &mut ItemStruct) -> Result<Vec<SerdeField>> {
    let Fields::Named(fields) = &mut input.fields else {
        return Ok(Vec::new());
    };

    let mut serde_fields = Vec::new();
    for field in &mut fields.named {
        if take_component_serde_attr(field)? {
            serde_fields.push(SerdeField {
                ident: field
                    .ident
                    .clone()
                    .expect("named fields were validated before collecting serde fields"),
                ty: field.ty.clone(),
                span: field.span(),
            });
        }
    }

    Ok(serde_fields)
}

fn take_component_serde_attr(field: &mut Field) -> Result<bool> {
    let mut has_serde_attr = false;
    let mut retained_attrs = Vec::with_capacity(field.attrs.len());

    for attr in field.attrs.drain(..) {
        if attr.path().is_ident("serde") {
            if has_serde_attr {
                return Err(syn::Error::new_spanned(
                    attr,
                    "duplicate `#[serde]` attribute; specify it only once per field",
                ));
            }

            parse_component_serde_attr(&attr)?;
            has_serde_attr = true;
        } else {
            retained_attrs.push(attr);
        }
    }

    field.attrs = retained_attrs;

    Ok(has_serde_attr)
}

fn parse_component_serde_attr(attr: &Attribute) -> Result<()> {
    match &attr.meta {
        Meta::Path(_) => Ok(()),
        Meta::List(_) => Err(syn::Error::new_spanned(
            attr,
            "`#[serde]` on component fields does not take arguments; implement `ComponentSerdeField` for the field type instead",
        )),
        Meta::NameValue(_) => Err(syn::Error::new_spanned(
            attr,
            "`#[serde]` on component fields does not take arguments; implement `ComponentSerdeField` for the field type instead",
        )),
    }
}

fn build_component_serde_impl(
    args: &ComponentArgs,
    input: &ItemStruct,
    serde_fields: &[SerdeField],
    jungle_core: &proc_macro2::TokenStream,
) -> proc_macro2::TokenStream {
    if args.noserde {
        return quote! {};
    }

    let ident = &input.ident;
    let helper_ident = format_ident!("__{}ComponentSerdeData", ident);
    let serialize_wrapper_ident = format_ident!("__{}ComponentSerializeField", ident);
    let deserialize_wrapper_ident = format_ident!("__{}ComponentDeserializeField", ident);
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    let deserialize_generics = add_deserialize_lifetime(&input.generics);
    let (deserialize_impl_generics, _, deserialize_where_clause) =
        deserialize_generics.split_for_impl();

    let helper_fields = serde_fields.iter().map(|field| {
        let ident = &field.ident;
        let field_ty = &field.ty;

        quote! {
            #ident: #deserialize_wrapper_ident<#field_ty>,
        }
    });

    let serialize_fields_len = serde_fields.len();
    let serialize_steps = serde_fields.iter().map(|field| {
        let ident = &field.ident;
        let field_name = ident.to_string();

        quote! {
            ::serde::ser::SerializeStruct::serialize_field(
                &mut state,
                #field_name,
                &#serialize_wrapper_ident(&self.#ident),
            )?;
        }
    });

    let deserialize_steps = serde_fields.iter().map(|field| {
        let ident = &field.ident;

        quote! {
            component.#ident = data.#ident.0;
        }
    });

    quote! {
        struct #serialize_wrapper_ident<'__a, T>(&'__a T);

        impl<'__a, T> ::serde::Serialize for #serialize_wrapper_ident<'__a, T>
        where
            T: #jungle_core::ecs::components::ComponentSerializeField,
        {
            fn serialize<S>(&self, serializer: S) -> ::std::result::Result<S::Ok, S::Error>
            where
                S: ::serde::Serializer,
            {
                <T as #jungle_core::ecs::components::ComponentSerializeField>::serialize_field(
                    self.0,
                    serializer,
                )
            }
        }

        struct #deserialize_wrapper_ident<T>(T);

        impl<'__de, T> ::serde::Deserialize<'__de> for #deserialize_wrapper_ident<T>
        where
            T: #jungle_core::ecs::components::ComponentDeserializeField<'__de>,
        {
            fn deserialize<D>(deserializer: D) -> ::std::result::Result<Self, D::Error>
            where
                D: ::serde::Deserializer<'__de>,
            {
                Ok(Self(
                    <T as #jungle_core::ecs::components::ComponentDeserializeField<'__de>>::deserialize_field(deserializer)?,
                ))
            }
        }

        #[derive(serde::Deserialize)]
        struct #helper_ident #impl_generics #where_clause {
            #(#helper_fields)*
            #[serde(skip)]
            __phantom: ::std::marker::PhantomData<fn() -> #ident #ty_generics>,
        }

        impl #deserialize_impl_generics ::serde::Deserialize<'__de> for #ident #ty_generics #deserialize_where_clause {
            fn deserialize<D>(deserializer: D) -> ::std::result::Result<Self, D::Error>
            where
                D: ::serde::Deserializer<'__de>,
            {
                let data: #helper_ident #ty_generics = ::serde::Deserialize::deserialize(deserializer)?;

                let mut component = Self::default();
                #(#deserialize_steps)*
                Ok(component)
            }
        }

        impl #impl_generics ::serde::Serialize for #ident #ty_generics #where_clause {
            fn serialize<S>(&self, serializer: S) -> ::std::result::Result<S::Ok, S::Error>
            where
                S: ::serde::Serializer,
            {
                let mut state = ::serde::ser::Serializer::serialize_struct(
                    serializer,
                    ::std::stringify!(#ident),
                    #serialize_fields_len,
                )?;
                #(#serialize_steps)*
                ::serde::ser::SerializeStruct::end(state)
            }
        }
    }
}

fn build_component_jaml_impl(
    args: &ComponentArgs,
    serde_fields: &[SerdeField],
    jungle_core: &proc_macro2::TokenStream,
) -> proc_macro2::TokenStream {
    if args.noserde {
        return quote! {
            fn from_jaml(_jaml: #jungle_core::asset::entity_tree::JamlComponent) -> ::std::option::Option<Self> {
                ::std::option::Option::None
            }

            fn to_jaml(&self) -> ::std::option::Option<#jungle_core::asset::entity_tree::JamlComponent> {
                ::std::option::Option::None
            }
        };
    }

    let serialize_steps = serde_fields.iter().map(|field| {
        let ident = &field.ident;
        let field_name = ident.to_string();
        let field_ty = &field.ty;

        quote! {
            jaml.insert(
                ::std::string::String::from(#field_name),
                <#field_ty as #jungle_core::ecs::components::ComponentSerdeField>::to_jaml(&self.#ident),
            );
        }
    });

    let deserialize_steps = serde_fields.iter().map(|field| {
        let ident = &field.ident;
        let field_ty = &field.ty;
        let field_name = ident.to_string();

        quote! {
            component.#ident = <#field_ty as #jungle_core::ecs::components::ComponentSerdeField>::from_jaml(
                jaml.get(#field_name)?.as_str(),
            )?;
        }
    });

    quote! {
        fn from_jaml(jaml: #jungle_core::asset::entity_tree::JamlComponent) -> ::std::option::Option<Self> {
            let mut component = Self::default();
            #(#deserialize_steps)*
            ::std::option::Option::Some(component)
        }

        fn to_jaml(&self) -> ::std::option::Option<#jungle_core::asset::entity_tree::JamlComponent> {
            let mut jaml = #jungle_core::asset::entity_tree::JamlComponent::new();
            #(#serialize_steps)*
            ::std::option::Option::Some(jaml)
        }
    }
}

fn add_deserialize_lifetime(generics: &syn::Generics) -> syn::Generics {
    let mut generics = generics.clone();
    generics.params.insert(0, parse_quote!('__de));
    generics
}

fn resolve_jungle_core_path(span: Span) -> Result<proc_macro2::TokenStream> {
    match crate_name("jungle-core") {
        Ok(FoundCrate::Itself) => Ok(quote!(crate)),
        Ok(FoundCrate::Name(name)) => {
            let ident = syn::Ident::new(&name, span);
            Ok(quote!(#ident))
        }
        Err(error) => Err(syn::Error::new(
            span,
            format!("failed to resolve `jungle-core` crate name: {error}"),
        )),
    }
}

fn build_storage_type(storage: &Path) -> Result<Type> {
    if storage.segments.is_empty() {
        return Err(syn::Error::new_spanned(
            storage,
            "`storage` must be a type path",
        ));
    }

    if storage
        .segments
        .iter()
        .any(|segment| !matches!(segment.arguments, PathArguments::None))
    {
        return Err(syn::Error::new_spanned(
            storage,
            "`storage` should not include generic arguments; write `storage = SparseComponentStorage`, not `storage = SparseComponentStorage<Self>`",
        ));
    }

    let mut storage_with_self = storage.clone();
    let last_segment = storage_with_self
        .segments
        .last_mut()
        .expect("checked above");
    last_segment.arguments = PathArguments::AngleBracketed(parse_quote!(<Self>));

    Ok(parse_quote!(#storage_with_self))
}

fn validate_component_struct(input: &ItemStruct) -> Result<&Field> {
    let Fields::Named(fields) = &input.fields else {
        return Err(syn::Error::new_spanned(
            &input.fields,
            "`#[component(...)]` requires a braced struct with an `entity: Entity` field",
        ));
    };

    fields
        .named
        .iter()
        .find(|field| field.ident.as_ref().is_some_and(|ident| ident == "entity"))
        .ok_or_else(|| {
            syn::Error::new_spanned(
                fields,
                "`#[component(...)]` requires a field named `entity`",
            )
        })
}

fn validate_entity_field_type(field: &Field) -> Result<()> {
    let Type::Path(TypePath { path, .. }) = &field.ty else {
        return Err(syn::Error::new_spanned(
            &field.ty,
            "the `entity` field must have type `Entity`",
        ));
    };

    let is_entity = path.segments.last().is_some_and(|segment| {
        segment.ident == "Entity" && matches!(segment.arguments, PathArguments::None)
    });

    if is_entity {
        Ok(())
    } else {
        Err(syn::Error::new_spanned(
            &field.ty,
            "the `entity` field must have type `Entity`",
        ))
    }
}
