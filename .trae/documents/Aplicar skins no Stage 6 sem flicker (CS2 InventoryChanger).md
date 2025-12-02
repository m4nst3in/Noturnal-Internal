## Objetivo
- Eliminar o flicker de skin ao comprar armas, aplicando fallback e atualização visual no primeiro Stage 6.
- Corrigir associação de owner/StatTrak via `m_iAccountID`.
- Garantir ordem correta de atualização para facas.
- Reaplicar imediatamente em caso de revalidação do servidor.

## Mudanças Principais
- Remover a dependência de tentativas antes de forçar `m_iItemIDHigh = -1`.
- Setar `m_iAccountID` para o XUID local na mesma aplicação.
- Reordenar o fluxo de facas: mudar `ItemDefinitionIndex`/`Subclass`, aplicar modelo e só então `UpdateSkin`.
- Adicionar verificação anti-revert no loop do Stage 6.

## Locais de Código
- `AndromedaClient/Features/CInventory/InventoryChanger.cpp`:
  - `ApplyToWeapon(...)`: antecipar a forçagem de fallback e `AccountID`; manter `PostDataUpdate → UpdateSkin → PostDataUpdate` imediato; remover gating por `m_ApplyRetryCount`.
  - `OnFrameStageNotify(int)`: reforçar owner check, sincronizar com loadout e:
    - Facas: `view->m_iItemDefinitionIndex` (novo), `UpdateSubclass`, `SetModel`, aplicar atributos fallback e atualizar skin.
    - Anti-revert: se `view->m_iItemIDHigh() != -1` e owner é local, reaplicar fallback e atualizar.
- `CS2/Hook/Hook_FrameStageNotify.cpp`: sem alteração (mantém chamada ao cliente antes/depósito do stage).

## Detalhamento das Alterações
- `ApplyToWeapon(C_CSWeaponBaseGun* weapon, const SkinSelection& sel)`:
  - Calcular `seed` e `wear` como já feito.
  - Definir imediatamente:
    - `econ->m_nFallbackPaintKit() = sel.paintkit`
    - `econ->m_nFallbackSeed() = seed`
    - `econ->m_flFallbackWear() = wear`
    - `if (sel.stattrak >= 0) econ->m_nFallbackStatTrak() = sel.stattrak`
  - Se `view->m_iItemIDHigh() != -1`, setar no mesmo tick:
    - `view->m_iItemIDHigh() = -1`
    - `view->m_iItemIDLow() = 0`
    - `view->m_iAccountID() = (uint32_t)(CCSPlayerInventory::Get()->GetOwner().ID() & 0xFFFFFFFF)`
    - `view->m_bDisallowSOC() = false`
  - Chamar `PostDataUpdate → C_CSWeaponBase_UpdateSkin(base, 1) → PostDataUpdate` sem esperar por contadores.

- `OnFrameStageNotify(int frameStage)`:
  - Manter varredura de entidades e owner check.
  - Para cada arma do owner local:
    - Encontrar `loadoutView` como já feito.
    - Definir `view` idêntico ao `loadoutView` quando existir (IDs e AccountID).
    - Facas:
      - `view->m_iItemDefinitionIndex()` para o `DefIndex` de faca equipado.
      - Chamar `C_CSWeaponBase_UpdateSubclass(wep)`.
      - `wep->SetModel(loadoutDef->m_pszModelName())` quando disponível.
      - Aplicar atributos fallback (paintkit/seed/wear/stattrak) após subclass/model.
    - Anti-revert:
      - Se `view->m_iItemIDHigh() != -1` e `view->m_iAccountID() == ownerAccountLow32`, forçar fallback (`m_iItemIDHigh = -1`, `m_iItemIDLow = 0`, `m_bDisallowSOC = false`) e atualizar skin imediatamente.
    - Finalizar com `PostDataUpdate → UpdateSkin → PostDataUpdate`.

## Associação de Owner
- Derivar `ownerAccountLow32` de `CCSPlayerInventory::Get()->GetOwner().ID() & 0xFFFFFFFF` e usar em `m_iAccountID`.

## Validação
- Eventos: manter limpeza em `item_purchase`/`round_start` (já implementada).
- Testes práticos:
  - Comprar AK/skin definida: sem flicker, skin nasce aplicada.
  - Comprar faca diferente: modelo/subclass atualizam antes do skin, sem artefatos.
  - Anti-revert: após ~200ms, se servidor reinstalar ID, o client reaplica fallback sem piscada.

## Riscos e Mitigações
- Evitar spam de `UpdateSkin`: executar uma vez por entidade no primeiro Stage pós-spawn; o loop já possui varredura controlada.
- Null checks e owner checks como já existentes permanecem.

## Resultado Esperado
- Skin aplicada no primeiro frame pós-compra sem flicker.
- StatTrak e owner corretos.
- Facas com modelo/subclass consistentes antes de pintar.
- Resiliência a revalidações do servidor.