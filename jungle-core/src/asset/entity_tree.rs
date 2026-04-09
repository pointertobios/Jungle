use std::collections::HashMap;

use anyhow::{Context, Result, anyhow, bail};
use bytes::Bytes;
use quick_xml::{
    Reader, Writer,
    events::{BytesEnd, BytesStart, BytesText, Event},
    name::QName,
};
use serde::{Deserialize, Serialize};

use crate::{asset::env::AssetEnv, ecs::component::ComponentId, game::Game};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum EntityTree {
    Entity(TagEntity),
    PrefabRef(TagPrefabRef),
    Prefab(TagPrefab),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TagEntity {
    pub entity_id: u64,
    pub name: String,
    pub enabled: bool,
    pub env: AssetEnv,
    pub components: Vec<TagComponent>,
    pub subentities: Vec<EntityTree>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TagComponent {
    pub component_id: ComponentId,
    pub component: ComponentData,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TagPrefabRef {
    pub name: String,
    pub source: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TagPrefab {
    pub name: String,
    pub source: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentData(Bytes);

pub type JamlComponent = HashMap<String, String>;

impl Default for ComponentData {
    fn default() -> Self {
        Self(Bytes::new())
    }
}

impl ComponentData {
    fn from_jaml(game: &impl Game, component_id: ComponentId, jaml: JamlComponent) -> Result<Self> {
        let serdeh = game
            .get_serdehelpers()
            .get(&component_id)
            .ok_or_else(|| anyhow!("component id {} is not registered in game", component_id))?;

        let bytes = serdeh
            .build_component_data_from_jaml(jaml)
            .ok_or_else(|| {
                anyhow!(
                    "component id {} does not support JAML deserialization",
                    component_id
                )
            })??;

        Ok(Self(bytes))
    }

    fn to_jaml(&self, game: &impl Game, component_id: ComponentId) -> Result<JamlComponent> {
        let serdeh = game
            .get_serdehelpers()
            .get(&component_id)
            .ok_or_else(|| anyhow!("component id {} is not registered in game", component_id))?;

        serdeh
            .build_jaml_from_component_data(&self.0)
            .ok_or_else(|| {
                anyhow!(
                    "component id {} does not support JAML serialization",
                    component_id
                )
            })?
    }
}

impl EntityTree {
    pub fn from_jaml(game: &impl Game, jaml: String) -> Result<Self> {
        let mut reader = Reader::from_str(&jaml);
        reader.config_mut().trim_text(true);

        loop {
            match reader.read_event().context("failed to parse JAML")? {
                Event::Start(start) => {
                    return Self::read_from_start(game, &mut reader, start);
                }
                Event::Empty(start) => {
                    return Self::read_from_empty(game, start);
                }
                Event::Decl(_) | Event::PI(_) | Event::DocType(_) | Event::Comment(_) => {}
                Event::Text(text) if text.as_ref().iter().all(u8::is_ascii_whitespace) => {}
                Event::Eof => bail!("empty JAML document"),
                event => bail!("unexpected top-level JAML event: {event:?}"),
            }
        }
    }

    pub fn to_jaml(&self, game: &impl Game) -> Result<String> {
        let mut writer = Writer::new_with_indent(Vec::new(), b' ', 4);

        writer
            .get_mut()
            .extend_from_slice(b"<?jaml version=\"1.0\"?>\n");
        self.to_jaml_inner(game, &mut writer)?;

        String::from_utf8(writer.into_inner()).context("failed to encode JAML as UTF-8")
    }

    fn read_from_start(
        game: &impl Game,
        reader: &mut Reader<&[u8]>,
        start: BytesStart<'_>,
    ) -> Result<Self> {
        match start.name().as_ref() {
            b"entity" => Ok(Self::Entity(TagEntity::from_jaml(game, reader, &start)?)),
            b"prefab-ref" => Ok(Self::PrefabRef(TagPrefabRef::from_start(&start)?)),
            b"prefab" => Ok(Self::Prefab(TagPrefab::from_start(&start)?)),
            other => bail!("unsupported root tag: {}", String::from_utf8_lossy(other)),
        }
    }

    fn read_from_empty(game: &impl Game, start: BytesStart<'_>) -> Result<Self> {
        match start.name().as_ref() {
            b"entity" => Ok(Self::Entity(TagEntity::from_empty(game, &start)?)),
            b"prefab-ref" => Ok(Self::PrefabRef(TagPrefabRef::from_start(&start)?)),
            b"prefab" => Ok(Self::Prefab(TagPrefab::from_start(&start)?)),
            other => bail!("unsupported root tag: {}", String::from_utf8_lossy(other)),
        }
    }

    fn to_jaml_inner<W: std::io::Write>(
        &self,
        game: &impl Game,
        writer: &mut Writer<W>,
    ) -> Result<()> {
        match self {
            Self::Entity(entity) => entity.to_jaml(game, writer),
            Self::PrefabRef(prefab_ref) => prefab_ref.to_jaml(writer),
            Self::Prefab(prefab) => prefab.to_jaml(writer),
        }
    }
}

impl TagEntity {
    fn from_empty(_game: &impl Game, start: &BytesStart<'_>) -> Result<Self> {
        Ok(Self {
            entity_id: parse_required_u64_attr(start, b"id")?,
            name: parse_required_string_attr(start, b"name")?,
            enabled: parse_bool_attr(start, b"enabled")?.unwrap_or(true),
            env: parse_env(start)?,
            components: Vec::new(),
            subentities: Vec::new(),
        })
    }

    fn from_jaml(
        game: &impl Game,
        reader: &mut Reader<&[u8]>,
        start: &BytesStart<'_>,
    ) -> Result<Self> {
        let mut entity = Self::from_empty(game, start)?;

        loop {
            match reader.read_event().context("failed to parse entity body")? {
                Event::Start(child) => match child.name().as_ref() {
                    b"subentity" => {
                        entity.subentities = read_subentities(game, reader)?;
                    }
                    b"component" => {
                        entity
                            .components
                            .push(TagComponent::from_jaml(game, reader, &child)?);
                    }
                    other => bail!(
                        "unsupported <entity> child tag: {}",
                        String::from_utf8_lossy(other)
                    ),
                },
                Event::Empty(child) => match child.name().as_ref() {
                    b"component" => {
                        entity
                            .components
                            .push(TagComponent::from_empty(game, &child)?);
                    }
                    b"subentity" => {}
                    other => bail!(
                        "unsupported <entity> child tag: {}",
                        String::from_utf8_lossy(other)
                    ),
                },
                Event::End(end) if end.name() == start.name() => return Ok(entity),
                Event::Text(text) if text.as_ref().iter().all(u8::is_ascii_whitespace) => {}
                Event::Comment(_) => {}
                Event::Eof => bail!("unexpected EOF while parsing <entity>"),
                event => bail!("unexpected event inside <entity>: {event:?}"),
            }
        }
    }

    fn to_jaml<W: std::io::Write>(&self, game: &impl Game, writer: &mut Writer<W>) -> Result<()> {
        let mut start = BytesStart::new("entity");
        let id = self.entity_id.to_string();
        let enabled = if self.enabled { "true" } else { "false" };

        start.push_attribute(("id", id.as_str()));
        start.push_attribute(("name", self.name.as_str()));
        start.push_attribute(("enabled", enabled));
        match self.env {
            AssetEnv::Client => start.push_attribute(("client_only", "true")),
            AssetEnv::Server => start.push_attribute(("server_only", "true")),
            AssetEnv::Both => {}
        }

        if self.subentities.is_empty() && self.components.is_empty() {
            writer
                .write_event(Event::Empty(start))
                .context("failed to write empty <entity> tag")?;
            return Ok(());
        }

        writer
            .write_event(Event::Start(start))
            .context("failed to write <entity> start tag")?;

        if !self.subentities.is_empty() {
            writer
                .write_event(Event::Start(BytesStart::new("subentity")))
                .context("failed to write <subentity> start tag")?;
            for subentity in &self.subentities {
                subentity.to_jaml_inner(game, writer)?;
            }
            writer
                .write_event(Event::End(BytesEnd::new("subentity")))
                .context("failed to write <subentity> end tag")?;
        }

        for component in &self.components {
            component.to_jaml(game, writer)?;
        }

        writer
            .write_event(Event::End(BytesEnd::new("entity")))
            .context("failed to write <entity> end tag")?;

        Ok(())
    }
}

impl TagComponent {
    fn from_empty(game: &impl Game, start: &BytesStart<'_>) -> Result<Self> {
        let component_name = parse_required_string_attr(start, b"type")?;
        let component_id = *game
            .get_name_to_type()
            .get(&component_name)
            .ok_or_else(|| anyhow!("unknown component type: {component_name}"))?;

        Ok(Self {
            component_id,
            component: ComponentData::default(),
        })
    }

    fn from_jaml(
        game: &impl Game,
        reader: &mut Reader<&[u8]>,
        start: &BytesStart<'_>,
    ) -> Result<Self> {
        let mut component = Self::from_empty(game, start)?;
        component.component = ComponentData::from_jaml(
            game,
            component.component_id,
            read_component_jaml(reader, start)?,
        )?;
        Ok(component)
    }

    fn to_jaml<W: std::io::Write>(&self, game: &impl Game, writer: &mut Writer<W>) -> Result<()> {
        let component_name = game
            .get_name_to_type()
            .iter()
            .find_map(|(name, id)| (*id == self.component_id).then_some(name.as_str()))
            .ok_or_else(|| {
                anyhow!(
                    "component id {} is not registered in game",
                    self.component_id
                )
            })?;

        let mut start = BytesStart::new("component");
        start.push_attribute(("type", component_name));

        let fields = self.component.to_jaml(game, self.component_id)?;
        if fields.is_empty() {
            writer
                .write_event(Event::Empty(start))
                .context("failed to write empty <component> tag")?;
            return Ok(());
        }

        writer
            .write_event(Event::Start(start))
            .context("failed to write <component> start tag")?;
        for (field_name, field_value) in fields {
            if field_value.is_empty() {
                writer
                    .write_event(Event::Empty(BytesStart::new(field_name.as_str())))
                    .with_context(|| format!("failed to write empty <{}> tag", field_name))?;
            } else {
                writer
                    .write_event(Event::Start(BytesStart::new(field_name.as_str())))
                    .with_context(|| format!("failed to write <{}> start tag", field_name))?;
                writer
                    .write_event(Event::Text(BytesText::new(field_value.as_str())))
                    .with_context(|| format!("failed to write <{}> text", field_name))?;
                writer
                    .write_event(Event::End(BytesEnd::new(field_name.as_str())))
                    .with_context(|| format!("failed to write <{}> end tag", field_name))?;
            }
        }
        writer
            .write_event(Event::End(BytesEnd::new("component")))
            .context("failed to write <component> end tag")?;

        Ok(())
    }
}

impl TagPrefabRef {
    fn from_start(start: &BytesStart<'_>) -> Result<Self> {
        Ok(Self {
            name: parse_required_string_attr(start, b"name")?,
            source: parse_required_string_attr(start, b"source")?,
        })
    }

    fn to_jaml<W: std::io::Write>(&self, writer: &mut Writer<W>) -> Result<()> {
        let mut start = BytesStart::new("prefab-ref");
        start.push_attribute(("name", self.name.as_str()));
        start.push_attribute(("source", self.source.as_str()));
        writer
            .write_event(Event::Empty(start))
            .context("failed to write <prefab-ref> tag")?;
        Ok(())
    }
}

impl TagPrefab {
    fn from_start(start: &BytesStart<'_>) -> Result<Self> {
        Ok(Self {
            name: parse_required_string_attr(start, b"name")?,
            source: parse_required_string_attr(start, b"source")?,
        })
    }

    fn to_jaml<W: std::io::Write>(&self, writer: &mut Writer<W>) -> Result<()> {
        let mut start = BytesStart::new("prefab");
        start.push_attribute(("name", self.name.as_str()));
        start.push_attribute(("source", self.source.as_str()));
        writer
            .write_event(Event::Empty(start))
            .context("failed to write <prefab> tag")?;
        Ok(())
    }
}

fn read_subentities(game: &impl Game, reader: &mut Reader<&[u8]>) -> Result<Vec<EntityTree>> {
    let mut subentities = Vec::new();

    loop {
        match reader
            .read_event()
            .context("failed to parse <subentity> body")?
        {
            Event::Start(start) => {
                subentities.push(EntityTree::read_from_start(game, reader, start)?);
            }
            Event::Empty(start) => {
                subentities.push(EntityTree::read_from_empty(game, start)?);
            }
            Event::End(end) if end.name().as_ref() == b"subentity" => return Ok(subentities),
            Event::Text(text) if text.as_ref().iter().all(u8::is_ascii_whitespace) => {}
            Event::Comment(_) => {}
            Event::Eof => bail!("unexpected EOF while parsing <subentity>"),
            event => bail!("unexpected event inside <subentity>: {event:?}"),
        }
    }
}

fn read_component_jaml(
    reader: &mut Reader<&[u8]>,
    component_tag: &BytesStart<'_>,
) -> Result<JamlComponent> {
    let mut component_jaml = JamlComponent::new();

    loop {
        match reader
            .read_event()
            .context("failed to parse <component> body")?
        {
            Event::Start(field) => {
                let field_name = decode_tag_name(field.name())?;
                let field_value = reader
                    .read_text(field.name())
                    .context("failed to read component field text")?
                    .into_owned();
                component_jaml.insert(field_name, field_value);
            }
            Event::Empty(field) => {
                let field_name = decode_tag_name(field.name())?;
                component_jaml.insert(field_name, String::new());
            }
            Event::End(end) if end.name() == component_tag.name() => return Ok(component_jaml),
            Event::Text(text) if text.as_ref().iter().all(u8::is_ascii_whitespace) => {}
            Event::Comment(_) => {}
            Event::Eof => bail!("unexpected EOF while parsing <component>"),
            event => bail!("unexpected event inside <component>: {event:?}"),
        }
    }
}

fn parse_required_string_attr(start: &BytesStart<'_>, attr_name: &[u8]) -> Result<String> {
    parse_optional_string_attr(start, attr_name)?.ok_or_else(|| {
        anyhow!(
            "missing required attribute {} on <{}>",
            String::from_utf8_lossy(attr_name),
            String::from_utf8_lossy(start.name().as_ref())
        )
    })
}

fn parse_required_u64_attr(start: &BytesStart<'_>, attr_name: &[u8]) -> Result<u64> {
    let value = parse_required_string_attr(start, attr_name)?;
    value.parse::<u64>().with_context(|| {
        format!(
            "invalid integer attribute {} on <{}>: {value}",
            String::from_utf8_lossy(attr_name),
            String::from_utf8_lossy(start.name().as_ref())
        )
    })
}

fn parse_bool_attr(start: &BytesStart<'_>, attr_name: &[u8]) -> Result<Option<bool>> {
    let Some(value) = parse_optional_string_attr(start, attr_name)? else {
        return Ok(None);
    };

    match value.as_str() {
        "true" => Ok(Some(true)),
        "false" => Ok(Some(false)),
        _ => bail!(
            "invalid boolean attribute {} on <{}>: {}",
            String::from_utf8_lossy(attr_name),
            String::from_utf8_lossy(start.name().as_ref()),
            value
        ),
    }
}

fn parse_optional_string_attr(start: &BytesStart<'_>, attr_name: &[u8]) -> Result<Option<String>> {
    for attr in start.attributes().with_checks(false) {
        let attr = attr.context("failed to parse XML attribute")?;
        if attr.key.as_ref() == attr_name {
            let value = attr
                .decode_and_unescape_value(start.decoder())
                .context("failed to decode XML attribute")?;
            return Ok(Some(value.into_owned()));
        }
    }

    Ok(None)
}

fn decode_tag_name(name: QName<'_>) -> Result<String> {
    std::str::from_utf8(name.as_ref())
        .map(str::to_owned)
        .context("failed to decode XML tag name")
}

fn parse_env(start: &BytesStart<'_>) -> Result<AssetEnv> {
    let client_only = parse_bool_attr(start, b"client_only")?.unwrap_or(false);
    let server_only = parse_bool_attr(start, b"server_only")?.unwrap_or(false);

    match (client_only, server_only) {
        (true, true) => bail!(
            "<{}> cannot set both client_only and server_only to true",
            String::from_utf8_lossy(start.name().as_ref())
        ),
        (true, false) => Ok(AssetEnv::Client),
        (false, true) => Ok(AssetEnv::Server),
        (false, false) => Ok(AssetEnv::Both),
    }
}
