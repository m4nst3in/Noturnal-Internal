## Contexto
- UI atual: `Visuals`, `Misc`, `Inventory` renderizados em `CAndromedaMenu::OnRenderMenu()` via ImGui.
- Configurações: `Settings::Visual` e `Settings::Colors::Visual` já integradas; `Settings::RageBot` existe sem UI; não há `Legit` definido.
- Persistência: `CSettingsJson` preparado, ainda sem serialização completa de todos campos.

## Objetivos
- Tornar o menu mais rápido de navegar, mais autoexplicativo e seguro.
- Acelerar descoberta de opções com busca, favoritos e presets.
- Melhorar consistência visual e acessibilidade (temas, fontes, escalas).

## Estrutura do Menu
- Tabs principais: `Visuals`, `Combate Avançado` (substitui “Rage”), `Assistido` (substitui “Legit”), `Misc`, `Inventory`, `Configurações`.
- Grupos colapsáveis por tema (ex.: HUD, ESP, Cores, Perfis) com ícones.
- Modo compacto: alterna entre layout detalhado e denso para telas menores.

## Navegação e Descoberta
- Barra de busca global: filtra controles por nome/descrição/tag.
- Favoritos: estrela em qualquer controle; seção “Favoritos” por tab.
- Histórico de mudanças: mostra últimas alterações com desfazer/refazer por sessão.
- Breadcrumbs/âncoras: links rápidos para grupos mais usados.

## Presets e Perfis
- Presets globais (ex.: “Baixo impacto”, “Equilíbrio”, “Alta visibilidade”).
- Perfis por arma/mapa/modo: aplica conjuntos de opções automaticamente.
- Importar/Exportar perfil em JSON; backup automático com versionamento simples.
- Perfil “Somente leitura”: evita alterações acidentais em sessões críticas.

## Ajuda e Explicações
- Tooltips ricos: descrição curta, impacto esperado e recomendações.
- “Por que isso?”: botão abre painel lateral com explicações e conflitos comuns.
- Detector de conflitos: marca em vermelho controles com combinações suspeitas.

## Visualizações e Pré-visualizações
- Curvas/gráficos para sliders (ex.: resposta versus valor).
- Preview de cores unificado com paletas e gradientes.
- Preview de HUD/ESP estático para checar posicionamento e tamanhos.

## Acessibilidade e Temas
- Seleção de tema (Indigo/Vermillion/Classic + alto contraste).
- Tamanho de fonte e escala da UI ajustáveis.
- Navegação por teclado, foco visível e atalhos configuráveis.

## Qualidade de Vida
- Botão “Pânico”: oculta o menu e restaura estado visual.
- Bloqueio por senha/pin para reabrir menu (opcional).
- “Travar controles”: impede edição enquanto bloqueado; ideal para partidas.

## Desempenho e Status
- Indicador de custo por grupo (aproximado) e aviso de impacto.
- Status bar: FPS, tempo de frame, versão do perfil carregado.
- Modo desempenho: desativa previews/efeitos supérfluos.

## Integração com Configs
- Normalizar chaveamento em `Settings::` para novos campos.
- Completar `LoadConfig/SaveConfig` para todos grupos/cores.
- Definir esquema mínimo (validação) para JSON.

## Arquitetura de Implementação
- Componentizar: `Page`, `Group`, `Field` (checkbox/combo/slider/color) com helpers de binding.
- Sistema de busca indexa `Field{ id, label, tags }` e navega por foco.
- Seção de presets/perfis usa API de `CSettingsJson` e UI de seleção.

## Próximos Passos (ordem sugerida)
1) Adicionar barra de busca e favoritos.
2) Reorganizar tabs e grupos com ícones e modo compacto.
3) Implementar presets/perfis (import/export, backup).
4) Tooltips ricos, explicações e detector de conflitos.
5) Temas, escalas e navegação por teclado.
6) Status/performance e modo desempenho.
7) Completar persistência/validação em `CSettingsJson`.

Confirme se quer que eu avance com essa implementação e em quais partes priorizar.