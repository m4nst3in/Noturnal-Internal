## Objetivo Geral
Redesenhar o menu ImGui para o tema "Noturnal" com identidade visual moderna (roxo/cinza/pretos/brancos), tamanho fixo 960x640, tabs reorganizadas com ícones, groupboxes maiores e estilizadas, nova fonte Montserrat em todo o menu e remoções de elementos inúteis.

## Onde alterar no projeto
- Janela/tabs/groupboxes: `AndromedaClient\GUI\CAndromedaMenu.cpp:12–127`
- Título atual da janela: `AndromedaClient\GUI\CAndromedaMenu.cpp:15`
- Pipeline de render: `AndromedaClient\CAndromedaGUI.cpp:119–175`
- Estilos/tema existentes: `AndromedaClient\CAndromedaGUI.cpp:227–521`
- Inicialização de fontes: `AndromedaClient\CAndromedaGUI.cpp:96–115`
- Backend de textura do atlas de fontes: `Common\Include\ImGui\imgui_impl_dx11.cpp:328–374`

## Dimensões e título
- Fixar tamanho do menu com `ImGui::SetNextWindowSize({960, 640}, ImGuiCond_Always)` em `AndromedaClient\GUI\CAndromedaMenu.cpp:14`.
- Remover collapse/minimizar (seta) usando `ImGuiWindowFlags_NoCollapse` no `ImGui::Begin` em `AndromedaClient\GUI\CAndromedaMenu.cpp:15`.
- Substituir o título padrão por cabeçalho customizado interno (sem barra de título), renderizado no topo do conteúdo com texto "Noturnal" em degrade roxo→branco, centralizado.
  - Implementar `RenderGradientText("Noturnal", pos, colStart, colEnd, font)` desenhando cada glyph com cor interpolada (aproximação de degrade por caractere) usando `ImDrawList::AddText`.

## Tabs e ícones
- Reorganizar tabs para: Rage, Legit, Visuals, Inventory, Misc, Settings em `AndromedaClient\GUI\CAndromedaMenu.cpp:17–127`.
- Etiquetas com ícones FontAwesome precedendo texto e espaçamento simétrico (ex.: `"<icon>  Rage"`).
- Preparar mapeamento dos ícones (ex.: bullseye para Rage, crosshair para Legit, eye para Visuals, box para Inventory, sliders para Settings, wrench para Misc).
- Carregar FontAwesome via array de bytes (que você fornecerá) e mesclar ao atlas com Montserrat.

## Groupboxes e layout
- Substituir groupboxes por wrapper reutilizável: `BeginGroupBox(title, size)`/`EndGroupBox()` baseado em `ImGui::BeginChild` + desenho customizado:
  - Fundo: preto fosco com leve opacidade (ex.: `ImVec4(0,0,0,0.85)`)
  - Borda: 2px, cinza escuro `#22272e` (sutil)
  - Bordas arredondadas (~12px)
  - Sombra/glow roxo muito sutil via `ImDrawList` (retângulos/blur leve)
  - Título centralizado no topo, fonte Montserrat branca/roxo claro, bold
  - Padding interno amplo: 24px
  - Espaçamento vertical uniforme entre elementos: 16px
- Layout em colunas com `ImGui::BeginTable` para garantir distribuição, sem sobreposição e sem separadores internos.
- Ampliar e reorganizar as opções existentes para caberem melhor em múltiplas groupboxes por tab (Visuals já possui ESP/HUD/Team Colors; replicar padrão e ampliar tamanhos em `AndromedaClient\GUI\CAndromedaMenu.cpp:22–115`).

## Tema Noturnal
- Criar `SetNoturnalStyle()` em `AndromedaClient\CAndromedaGUI.cpp` definindo:
  - `ImGuiStyle` (rounding global 12, `FrameRounding`/`GrabRounding`) e dimensões
  - Paleta roxo/cinza/pretos/brancos: `ImGuiCol_WindowBg`, `ChildBg`, `Border`, `FrameBg`, `FrameBgHovered/Active`, `TitleBgActive` (se usar barra), `Text`, `CheckMark`, `SliderGrab`, etc.
- Modificar `UpdateStyle` em `AndromedaClient\CAndromedaGUI.cpp:511–521` para fixar Noturnal (remover troca de temas Vermillion/Indigo/Classic Steam). 

## Fonte Montserrat + ícones por array de bytes
- Criar função `LoadFontFromBytes(const unsigned char* montserrat_data, size_t montserrat_size, const unsigned char* fa_data, size_t fa_size, float size_px)` em `AndromedaClient\CAndromedaGUI.cpp`.
  - Substituir `io.Fonts->AddFontFromFileTTF` em `AndromedaClient\CAndromedaGUI.cpp:111` por `AddFontFromMemoryTTF` (Montserrat Regular como default).
  - Mesclar FontAwesome (via `ImFontConfig::MergeMode = true`) com ranges apropriados.
  - Indicar claramente onde inserir os arrays de bytes (na própria função ou em bloco estático definido no arquivo).
- Atualizar todo o menu para usar a Montserrat (fonte padrão do contexto ImGui).

## Remoções e limpeza
- Remover opções de personalização inúteis no menu (switches de tema, alpha slider global etc.) presentes em `AndromedaClient\CAndromedaMenu.cpp:12` e `AndromedaClient\CAndromedaGUI.cpp:511–521`.
- Remover qualquer botão/seta manual de minimizar (além do `NoCollapse`).

## Melhorias criativas
- Hover suave: interpolação de cor/alpha em elementos focados com pequena transição temporal.
- Micro animação de tabs: cor de ícone/texto com fade ao selecionar e pequeno deslocamento vertical (2px) com easing.
- Fade-in da janela ao abrir (alpha do style → 1.0 usando `ImGui::GetTime`).
- Efeito de foco nas groupboxes (levíssima intensificação do glow roxo ao hover do cabeçalho).

## Entregáveis
- Menu 960x640, título custom "Noturnal" com degrade.
- Tabs reorganizadas com ícones e labels simétricos.
- Groupboxes maiores, estilos exatos (fundo/preto fosco, borda 2px #22272e, raio 12, padding 24, spacing 16, sombra sutil).
- Tema Noturnal aplicado globalmente; outros temas removidos.
- Fonte Montserrat carregada por array de bytes e aplicada; FontAwesome mesclado.

## Validação
- Abrir menu e verificar tamanho fixo, ausência de seta de minimizar, cabeçalho centralizado com degrade.
- Confirmar legibilidade/contraste de textos e interações (hover/ativos).
- Verificar que groupboxes não se sobrepõem e respeitam layout em colunas.
- Checar que todos textos/ícones usam Montserrat + FontAwesome.

## Observações
- Se quiser manter título de janela padrão, podemos manter `CHEAT_NAME` e renderizar o cabeçalho interno adicional; porém, para controle total do visual, a recomendação é ocultar a barra de título e usar o cabeçalho customizado.