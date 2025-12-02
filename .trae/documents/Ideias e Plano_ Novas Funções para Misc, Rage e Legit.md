## Objetivos
- Propor opções novas e úteis para Misc, Rage e Legit com foco em vantagem, UX e segurança.
- Planejar uma primeira leva de implementação que não quebre o que já existe e seja configurável por arma/estado.

## Ideias por Categoria
### Misc (qualidade de vida e utilitários)
- Auto-buy avançado: perfis por lado/arma, prioridades e fallback.
- Grenade prediction com visual de arco/impacto e infos de dano provável.
- Movement helpers: edge-jump, long-jump assist, fast-stop inteligente, bhop com limite de variância.
- Auto-strafe modos: padrão, W-tap, strafe dinâmico por ângulo.
- Clutch mode: reduz som, oculta visuals chamativos, otimiza teclas e HUD minimalista.
- Bomb/defuse HUD: tempo exato, viabilidade do defuse com kit/sem kit, e sinalizador de fake.
- Keybind manager com perfis e indicadores on-screen (toggle/hold, mostra estado).
- Panic key (ocultar tudo), e modo streaming (remove overlays sensíveis).

### Rage (HVH e combate agressivo)
- AutoPeek v2 com prefire scan (traça rota segura + micro-strafe) e retorno automático.
- Double Tap com lógica de recharge inteligente e indicadores (cor, barra, ícone).
- Anti-aim builder por estado (parado/movendo/ar/peek): yaw base, jitter, desync, body lean.
- Freestand + edge-yaw: escolhe lado com menor dano previsto e quebra ângulos em borda.
- Resolver heurístico com histórico (body update, LC breaks) e fallback por dano.
- Hitbox/spot selection dinâmico por risco (head/body/pelvis), safe points e min-dmg flexível.
- Hitchance adaptativo por velocidade do alvo, distância e penetração.
- Lag compensation tools: evitar backtrack traps, prioridade por tick potência.

### Legit (discreto e natural)
- Aimbot por arma: FOV dinâmico (velocidade/alvo), smoothing adaptativo e curva exponencial.
- RCS híbrido: multiplicadores por fase do spray e compensação do primeiro tiro.
- Triggerbot com filtros (hitbox, visibilidade), delay humano e burst control.
- Backtrack legítimo com limite em ms e filtro por visibilidade.
- Assist de mira em peek: micro-ajuste no first shot, auto-fast stop sutil.
- Recoil crosshair, radar melhorado (info de bomb, granadas ativas) e sound ESP suave.
- Auto-scope inteligente para snipers e des-zoom após tiro conforme distancia.

## Priorização para Primeira Entrega (v1)
- Misc: grenade prediction + bomb/defuse HUD + keybind manager básico.
- Rage: AutoPeek v2 + Double Tap com recharge lógico + Anti-aim builder por estado.
- Legit: Aimbot por arma com FOV/smoothing dinâmicos + Triggerbot avançado + RCS híbrido.

## Integração de Menu
- Categorias: Misc, Rage, Legit; subpáginas por arma/estado.
- Controles: toggles, sliders (FOV/smoothing/min-dmg/hitchance), selects (modos), binds (toggle/hold), perfis.
- Indicadores on-screen configuráveis (cores/tamanho/posição); search bar para opções; tooltips curtas.
- Perfis: salvar/carregar, import/export; presets de fábrica.

## Implementação Técnica (resumo)
- Config: expandir schema por arma/estado, persistência e migração suave das configs antigas.
- Input/binds: sistema de binds com estados (toggle/hold), indicadores e conflito seguro.
- Visuals: overlay para grenade path, bomb/defuse HUD e indicadores DT/AutoPeek; encaixar onde os chams e HUD já vivem (ex.: você abriu `cstrike/features/visuals/chams.h:131`, podemos manter padrões de desenho/cores).
- Misc movement: hooks de movimento para edge/long-jump, fast-stop e auto-strafe; respeitar flags do player e previsões.
- Rage core: módulo de AutoPeek com pathing simples, DT com recharge por condições (tickbase), AA builder com estados e resolver modular.
- Legit core: per-weapon tables, FOV/smoothing dinâmicos, RCS por fase, triggerbot com heurísticas; backtrack dentro de limites.

## Verificação e Segurança
- Overlays de debug: mostra FOV, hitchance e caminho do AutoPeek; toggláveis.
- Testes em modo offline/lan; validação de performance e sem travamentos.
- Modo streaming/panic para ocultar rapidamente; sem logar dados sensíveis.

## Entregáveis
- Novas opções no menu e configs associadas.
- Três features por categoria funcionando (v1) com indicadores básicos.
- Documentação curta das opções e presets iniciais.

Confirma quais ideias e prioridades você quer para começarmos a implementar na v1 (podemos ajustar a lista conforme sua preferência).