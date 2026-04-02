use proc_macro::TokenStream;

use proc_macro_crate::{FoundCrate, crate_name};
use proc_macro2::Span;
use quote::{ToTokens, quote};
use syn::spanned::Spanned;
use syn::{
    Field, Fields, Item, ItemStruct, LitBool, Path, PathArguments, Result, Type, TypePath,
    parse::Parser, parse_macro_input, parse_quote,
};

struct ComponentArgs {
    storage: Path,
    exclusive: bool,
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

    let parser = syn::meta::parser(|meta| {
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
            "unknown component argument `{key}`; expected `storage = StorageType` or `exclusive = true/false`"
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

    Ok(ComponentArgs { storage, exclusive })
}

fn expand_component(args: ComponentArgs, input: ItemStruct) -> Result<proc_macro2::TokenStream> {
    let entity_field = validate_component_struct(&input)?;
    validate_entity_field_type(entity_field)?;

    let ident = &input.ident;
    let storage = build_storage_type(&args.storage)?;
    let exclusive = args.exclusive;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    let jungle_core = resolve_jungle_core_path(input.span())?;

    Ok(quote! {
        #input

        impl #impl_generics #jungle_core::ecs::component::Component for #ident #ty_generics #where_clause {
            type Storage = #storage;

            const EXCLUSIVE: bool = #exclusive;

            fn type_id(&self) -> ::std::any::TypeId {
                ::std::any::TypeId::of::<Self>()
            }

            fn type_name(&self) -> &'static str {
                ::std::stringify!(#ident)
            }

            fn downcast_unchecked<T>(&self) -> &T {
                unsafe { &*(self as *const Self).cast::<T>() }
            }

            fn downcast_unchecked_mut<T>(&mut self) -> &mut T {
                unsafe { &mut *(self as *mut Self).cast::<T>() }
            }

            fn entity(&self) -> #jungle_core::ecs::entity::Entity {
                self.entity
            }
        }
    })
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
