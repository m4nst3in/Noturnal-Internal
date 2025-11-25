# Objetivo
Criar um Skin Changer completo para armas na aba "Inventory", com suporte a Knifes, Gloves, Agents, Music kits e Badges, presets/randomizer, sincronização de configs e auto-apply, além de preview rápido. Ajustar o menu para `800x600`.

## UI (Inventory)
1. Redimensionar janela principal para `800x600` (ImGui `SetNextWindowSize`) e layout em dois painéis.
2. Painel esquerdo: seleção por categoria (Pistols, SMGs, Rifles, Heavy, Knives, Gloves, Agents, Music, Badges) e por arma.
3. Painel direito: lista de skins da arma selecionada (com busca), sliders/inputs de `Wear`, `Seed`, `StatTrak`, input `Nametag` e botões `Aplicar`, `Salvar preset`, `Randomizar`.
4. Indicadores visuais de aplicação e de auto-apply.

## Dados (Schema/Econ)
1. Usar `CEconItemSystem::GetEconItemSchema()` para obter:
   - Paint kits: `CEconItemSchema::GetPaintKits()` (`CPaintKit{nID, sName, sDescriptionTag, nRarity}`)
   - Music kits: `GetMusicKitDefinitions()`
   - Item definitions: `GetSortedItemDefinitionMap()` (`CEconItemDefinition`: `IsWeapon`, `IsKnife`, `IsGlove`, `IsAgent`)
2. Montar catálogo por categoria usando `ItemDefinitionIndex` (`GameClient/CL_ItemDefinition.hpp`) e filtrar por `IsWeapon/IsKnife/IsGlove/IsAgent`.
3. Mapear "todas skins disponíveis para a arma":
   - Preferir filtro por `CPaintKit::sDescriptionTag` ou nome que contenha o nome da arma.
   - Fallback: lista completa de paint kits com ordenação por raridade/compatibilidade conhecida.

## Aplicação de Skins
1. Criar/editar `CEconItem` via `CEconItem::Create()` e setar atributos:
   - DefIndex (arma/knife/glove/agent), AccountID, IDs únicos (`CCSPlayerInventory::GetHighestIDs()`).
   - Atributos: `SetPaintKit`, `SetPaintSeed`, `SetPaintWear`, `SetStatTrak`, `SetStatTrakType`, `SetMusicId`.
2. Adicionar ao inventário: `CCSPlayerInventory::AddEconItem(CEconItem*)` e equipar com `CCSInventoryManager::EquipItemInLoadout(team, slot, itemID)`.
3. Para Gloves/Knives/Agents/Music/Badges, usar `CEconItemDefinition::IsGlove/IsKnife/IsAgent` e slots correspondentes de loadout.

## Presets e Randomizer
1. Persistir em `Settings::Inventory` (por arma): paintkit, seed, wear, stattrak, nametag.
2. Presets nomeados: salvar/carregar conjuntos; randomizer por arma ou global.
3. Import/Export por JSON junto ao sistema de configs existente.

## Auto-Apply e Sincronização
1. Auto-apply ao entrar no servidor: hook inicialização do cliente para re-equipar.
2. Reaplicar ao trocar time/classe (quando necessário) e ao abrir o menu se estado não-setado.

## Preview Rápido
1. Pré-visualização: usar `CEconItemDefinition::m_pszIconName/m_pszModelName` para mostrar ícone/miniatura no painel direito.
2. Fallback: preview textual com nome da skin e cor/raridade.

## Integrações no Código
- UI: `AndromedaClient/GUI/CAndromedaMenu.cpp` (aba Inventory) e `CAndromedaGUI` (tamanho da janela).
- Settings: expandir `AndromedaClient/Settings/Settings.hpp` com `Settings::Inventory` (por arma e presets).
- Features: criar módulo `AndromedaClient/Features/CInventoryChanger` para criar/aplicar itens.
- Schema: reutilizar `CS2/SDK/Econ/*` (`CEconItem`, `CEconItemDefinition`, `CEconItemSchema`, `CEconItemSystem`).
- Inventory Manager: `CCSInventoryManager`, `CCSPlayerInventory` para equipar/adicionar.

## Entregas (primeira fase)
1. UI de Inventory completa e redimensionamento para `800x600`.
2. Catálogo por arma com lista de todas skins disponíveis + busca.
3. Aplicação de skin com atributos (paintkit, wear, seed, StatTrak, nametag).
4. Presets simples e randomizer por arma.
5. Auto-apply básico e preview de ícone.

## Próximas Fases
- Suporte completo a Knives/Gloves/Agents/Music/Badges.
- Presets globais, import/export, preview 3D, indicadores avançados.

Confirme para eu começar a implementar a Fase 1 imediatamente, seguindo o estilo e padrões do projeto.