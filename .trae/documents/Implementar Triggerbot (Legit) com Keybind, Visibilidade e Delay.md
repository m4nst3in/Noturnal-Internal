## Visão Geral
- Adicionar um Triggerbot "Legit" simples e funcional: keybind configurável, checagem de visibilidade, delay de tiro, filtros básicos de arma/aliado.
- Integrar no ciclo `OnCreateMove` e expor controles no menu na aba "Legit".

## Integração no Ciclo de Jogo
- Ponto de execução por tick: `Hook_CreateMove` chama `CAndromedaClient::OnCreateMove(...)` (CS2/Hook/Hook_CreateMove.cpp), que já despacha para Visual em `AndromedaClient/CAndromedaClient.cpp:65-69`.
- Alterar `CAndromedaClient::OnCreateMove(...)` para também chamar `GetTriggerBot()->OnCreateMove(pInput, pUserCmd)` logo após Visual.

## Estrutura da Feature
- Criar `AndromedaClient/Features/CTriggerBot/CTriggerBot.hpp/.cpp` seguindo o padrão de `CVisual`:
  - Classe `CTriggerBot` com método `OnCreateMove(CCSGOInput* pInput, CUserCmd* pUserCmd)`.
  - Instância global estática e accessor `auto GetTriggerBot() -> CTriggerBot*;`.

## Lógica do Triggerbot
- Pré-condições:
  - Local vivo: `CL_Players::IsLocalPlayerAlive()` (GameClient/CL_Players.cpp:41-53).
  - Arma ativa válida e pronta para atirar:
    - Obter arma: `GetCL_Weapons()->GetLocalActiveWeapon()` (GameClient/CL_Weapons.cpp:10-28).
    - Filtrar tipos: ignorar faca/granada via `GetLocalWeaponType()` (GameClient/CL_Weapons.cpp:55-63) e enum `CSWeaponType_t` (CS2/SDK/Types/CBaseTypes.hpp).
    - Pronto para atirar: `clip>0`, `!m_bInReload()`, `controller->m_nTickBase() >= weapon->m_nNextPrimaryAttackTick()` (CS2/SDK/Types/CEntityData.hpp:523-549, 297-304).
- Keybind:
  - Checar `ImGui::IsKeyDown(Settings::Legit::TriggerbotKey)` com roteamento global (ver ImGui inputs em Common/Include/ImGui/imgui.h:969-979, 1548-1555).
  - Desativar quando o menu captura teclado: `ImGui::GetIO().WantCaptureKeyboard`.
- Aquisição de alvo sob mira:
  - Raycast do olho pela direção da mira: `GetCL_Trace()->TraceToBoneEntity(pInput, nullptr, nullptr)` retorna par `(bone_hash, C_BaseEntity*)` (GameClient/CL_Trace.cpp:20-59).
  - Validar `ent->IsPlayerPawn()` e cast para `C_CSPlayerPawn*` (CS2/SDK/Types/CEntityData.cpp; uso do padrão em GameClient/CL_VisibleCheck.cpp).
  - Comparar equipes: `controller->m_iTeamNum() != localController->m_iTeamNum()` (AndromedaClient/Features/CVisual/CVisual.cpp:66-68).
  - Visibilidade opcional: `GetCL_VisibleCheck()->IsPlayerPawnVisible(pawn)` (GameClient/CL_VisibleCheck.cpp:24-66).
- Delay de disparo:
  - Usar tempo de jogo: `Pointers::GlobalVarsBase()->m_flCurrentTime()` e armazenar `m_flAcquireTime` ao primeiro frame com alvo válido (CS2/SDK/Update/CGlobalVarsBase.hpp; CS2/SDK/SDK.cpp resolve ponteiro).
  - Disparar quando `(currentTime - m_flAcquireTime) >= Settings::Legit::TriggerbotDelayMs / 1000.f`.
- Acionamento do tiro:
  - Executar `GetCL_Bypass()->SetAttack(pUserCmd, true)` para pressionar `IN_ATTACK` e injetar subtick (GameClient/CL_Bypass.cpp:90-104).
  - Soltar no tick seguinte para um "tap" limpo: rastrear `m_nLastAttackTick` e chamar `GetCL_Bypass()->SetDontAttack(pUserCmd, false)` quando `GlobalVars->m_nTickCount()` avançar (CS2/SDK/Update/CGlobalVarsBase.hpp; GameClient/CL_Bypass.cpp:106-118).
  - Para armas automáticas, manter comportamento de "tap" simples (uma bala por aquisição). Podemos evoluir para "hold" com opção futura.

## Configurações (Settings)
- Adicionar um namespace `Settings::Legit` com campos do Triggerbot:
  - `inline bool TriggerbotEnabled = true;`
  - `inline ImGuiKey TriggerbotKey = ImGuiKey_LeftAlt;`
  - `inline int TriggerbotDelayMs = 80;` (0–300)
  - `inline bool TriggerbotOnlyVisible = true;`
  - `inline bool TriggerbotIgnoreTeam = true;`
  - `inline bool TriggerbotFilterKnivesGrenades = true;`

## UI no Menu (Legit)
- Na aba "Legit" (`AndromedaClient/GUI/CAndromedaMenu.cpp:26-29`), criar um grupo "Triggerbot":
  - `Checkbox("Enable", &Settings::Legit::TriggerbotEnabled)`.
  - "Keybind": botão "Bind" que captura próxima tecla pressionada. Durante captura, iterar `ImGuiKey_NamedKey_BEGIN..END` ou lista curta (MouseLeft/Right/Middle, LeftAlt, LeftShift, F1..F6) e salvar no `Settings::Legit::TriggerbotKey`. Mostrar `ImGui::GetKeyName(...)` do key atual.
  - `SliderInt("Delay (ms)", &Settings::Legit::TriggerbotDelayMs, 0, 300)`.
  - `Checkbox("Apenas visível", &Settings::Legit::TriggerbotOnlyVisible)`.
  - `Checkbox("Ignorar aliados", &Settings::Legit::TriggerbotIgnoreTeam)`.
  - `Checkbox("Ignorar facas/granadas", &Settings::Legit::TriggerbotFilterKnivesGrenades)`.

## Verificações e Segurança
- Evitar conflito com menu: não acionar se `io.WantCaptureKeyboard` true ou se menu visível.
- Respeitar estados do jogo: não atacar quando recarregando, sem munição, arma inválida.
- Usar ray/visibilidade já disponíveis, sem offsets adicionais.

## Offsets Necessários
- Todos os campos usados já existem no SDK do projeto:
  - `m_nNextPrimaryAttackTick`, `m_iClip1`, `m_bInReload` (CS2/SDK/Types/CEntityData.hpp:523-549).
  - `m_nTickBase` do controller (CS2/SDK/Types/CEntityData.hpp:297-304).
  - Globais de tempo/tick: `m_flCurrentTime`, `m_nTickCount` (CS2/SDK/Update/CGlobalVarsBase.hpp).
- Caso algum campo esteja divergente na sua build, me informe os offsets específicos que atualizo os acessos.

## Arquivos a Criar/Editar
- Criar: `AndromedaClient/Features/CTriggerBot/CTriggerBot.hpp`, `AndromedaClient/Features/CTriggerBot/CTriggerBot.cpp`.
- Editar: `AndromedaClient/CAndromedaClient.cpp` para chamar o triggerbot.
- Editar: `AndromedaClient/Settings/Settings.hpp` para adicionar `Settings::Legit` e campos do Triggerbot.
- Editar: `AndromedaClient/GUI/CAndromedaMenu.cpp` para UI de configuração.

## Resultado Esperado
- Triggerbot 100% funcional: pressione o keybind com a mira sobre um inimigo visível e, após o delay configurado, o disparo é acionado de forma estável.
- Comportamento seguro e simples: sem tiros em aliados (se ativo), sem facas/granadas, sem conflito com o menu.

Confirma que posso implementar conforme o plano? Se preferir um keybind padrão diferente, diga qual; caso precise, posso ajustar para modo "toggle" em vez de "hold".