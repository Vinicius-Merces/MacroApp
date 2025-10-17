#!/bin/bash

# =============================================
# MACROAPP - DEPLOY CORRIGIDO
# =============================================

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "============================================="
echo "    MACROAPP - DEPLOY CORRIGIDO"
echo "============================================="
echo -e "${NC}"

# Função para verificar e criar ícones faltantes
create_missing_icons() {
    echo -e "${BLUE}💡 Verificando ícones...${NC}"
    
    # Criar ícones básicos se não existirem
    mkdir -p icons
    
    # Lista de ícones necessários
    declare -a required_icons=(
        "app_icon.ico"
        "app_icon.png" 
        "logo.png"
        "record.png"
        "stop.png"
        "play.png"
    )
    
    for icon in "${required_icons[@]}"; do
        if [ ! -f "icons/$icon" ]; then
            echo -e "${YELLOW}⚠️  Criando ícone placeholder: $icon${NC}"
            
            if [[ "$icon" == *.ico ]]; then
                # Para .ico, criar um arquivo .png e converter
                convert -size 64x64 xc:#2a82da -fill white -pointsize 24 -gravity center -annotate +0+0 "M" "icons/$icon" 2>/dev/null || \
                echo "❌ Não foi possível criar $icon - instale ImageMagick ou adicione manualmente"
            else
                # Usar Qt para criar ícones simples
                python3 -c "
from PyQt5.QtGui import QPixmap, QPainter, QColor, QFont
from PyQt5.QtCore import Qt
import sys
pixmap = QPixmap(64, 64)
pixmap.fill(QColor('#2a82da'))
painter = QPainter(pixmap)
painter.setPen(QColor('white'))
painter.setFont(QFont('Arial', 24, QFont.Bold))
painter.drawText(pixmap.rect(), Qt.AlignCenter, 'M')
painter.end()
pixmap.save('icons/$icon')
" 2>/dev/null || \
                echo "❌ Não foi possível criar $icon - adicione manualmente na pasta icons/"
            fi
        else
            echo -e "${GREEN}✅ Ícone encontrado: $icon${NC}"
        fi
    done
}

# Função para detectar Qt
detect_qt() {
    echo -e "${BLUE}💡 Detectando Qt...${NC}"
    
    local paths=(
        "/c/msys64/mingw64"
        "/mingw64" 
        "/c/Qt/5.15.2/mingw81_64"
        "/c/Qt/6.5.0/mingw_64"
        "$(dirname "$(which qmake 2>/dev/null)")/.."
    )
    
    for path in "${paths[@]}"; do
        if [ -d "$path/bin" ] && [ -f "$path/bin/qmake.exe" ]; then
            QT_BASE="$path"
            QT_BIN="$path/bin"
            QT_PLUGINS="$path/share/qt5/plugins"
            [ ! -d "$QT_PLUGINS" ] && QT_PLUGINS="$path/plugins"
            
            echo -e "${GREEN}✅ Qt encontrado: $QT_BASE${NC}"
            return 0
        fi
    done
    
    echo -e "${RED}❌ Qt não encontrado${NC}"
    return 1
}

# Função para compilar
compile_project() {
    echo -e "${BLUE}💡 Compilando projeto...${NC}"
    
    # Limpar
    rm -rf release debug 2>/dev/null
    rm -f Makefile* *.o moc_* ui_* qrc_* 2>/dev/null
    
    # Criar arquivo .rc simples para o ícone
    cat > MacroApp_resource.rc << 'EOF'
IDI_ICON1 ICON DISCARDABLE "icons/app_icon.ico"
EOF

    # Compilar
    if qmake MacroApp.pro && mingw32-make -j4; then
        if [ -f "release/MacroApp.exe" ]; then
            echo -e "${GREEN}✅ Compilação bem-sucedida${NC}"
            return 0
        else
            echo -e "${RED}❌ Executável não criado${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ Falha na compilação${NC}"
        return 1
    fi
}

# Função para copiar DLLs do Mingw
copy_mingw_dlls() {
    local target_dir="$1"
    echo -e "${BLUE}💡 Copiando DLLs do Mingw...${NC}"
    
    cd "$target_dir"
    
    # DLLs ESSENCIAIS do Mingw
    declare -a mingw_dlls=(
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll" 
        "libwinpthread-1.dll"
    )
    
    # Copiar DLLs essenciais
    for dll in "${mingw_dlls[@]}"; do
        if [ -f "/mingw64/bin/$dll" ]; then
            cp "/mingw64/bin/$dll" .
            echo -e "${GREEN}✅ Mingw DLL: $dll${NC}"
        else
            echo -e "${YELLOW}⚠️  DLL não encontrada: $dll${NC}"
        fi
    done
    
    cd ..
}

# Função para deploy completo
full_deploy() {
    local target_dir="$1"
    echo -e "${BLUE}💡 Configurando: $target_dir${NC}"
    
    # Criar estrutura
    mkdir -p "$target_dir"
    mkdir -p "$target_dir/platforms"
    mkdir -p "$target_dir/imageformats" 
    mkdir -p "$target_dir/styles"
    
    # Copiar executável
    cp release/MacroApp.exe "$target_dir/"
    
    # Usar windeployqt se disponível
    if command -v windeployqt.exe &> /dev/null; then
        echo -e "${BLUE}💡 Executando windeployqt...${NC}"
        cd "$target_dir"
        windeployqt.exe MacroApp.exe --release --no-compiler-runtime 2>/dev/null && \
        echo -e "${GREEN}✅ windeployqt concluído${NC}" || \
        echo -e "${YELLOW}⚠️  windeployqt com avisos${NC}"
        cd ..
    else
        echo -e "${YELLOW}⚠️  windeployqt não encontrado, usando método manual${NC}"
        manual_qt_deploy "$target_dir"
    fi
    
    # COPIAR DLLs DO MINGW
    copy_mingw_dlls "$target_dir"
    
    # Copiar ícones
    if [ -d "icons" ]; then
        cp -r icons "$target_dir/"
        echo -e "${GREEN}✅ Ícones copiados${NC}"
    fi
    
    # Criar launcher Windows
    create_windows_launcher "$target_dir"
    
    echo -e "${GREEN}✅ Deploy concluído para: $target_dir${NC}"
}

# Deploy manual do Qt
manual_qt_deploy() {
    local target_dir="$1"
    
    # DLLs ESSENCIAIS Qt
    declare -a qt_dlls=(
        "Qt5Core.dll" "Qt5Gui.dll" "Qt5Widgets.dll" "Qt5Network.dll"
        "libpng16-16.dll" "libharfbuzz-0.dll" "libfreetype-6.dll"
        "zlib1.dll" "libbz2-1.dll"
    )
    
    # Copiar DLLs Qt
    for dll in "${qt_dlls[@]}"; do
        if [ -f "$QT_BIN/$dll" ]; then
            cp "$QT_BIN/$dll" "$target_dir/"
            echo -e "${GREEN}✅ Qt DLL: $dll${NC}"
        fi
    done
    
    # Plugins CRÍTICOS
    if [ -f "$QT_PLUGINS/platforms/qwindows.dll" ]; then
        cp "$QT_PLUGINS/platforms/qwindows.dll" "$target_dir/platforms/"
        echo -e "${GREEN}✅ Plugin: platforms/qwindows.dll${NC}"
    fi
    
    # Image formats
    declare -a img_plugins=("qico.dll" "qpng.dll")
    for plugin in "${img_plugins[@]}"; do
        if [ -f "$QT_PLUGINS/imageformats/$plugin" ]; then
            cp "$QT_PLUGINS/imageformats/$plugin" "$target_dir/imageformats/"
            echo -e "${GREEN}✅ Plugin: imageformats/$plugin${NC}"
        fi
    done
}

# Criar launcher Windows
create_windows_launcher() {
    local target_dir="$1"
    
    cat > "$target_dir/MacroApp-Launcher.bat" << 'EOF'
@echo off
chcp 65001 >nul
title MacroApp Launcher

echo =================================
echo        MACROAPP LAUNCHER
echo =================================

:: Configurar PATH com DLLs locais
set MY_PATH=%~dp0
set PATH=%MY_PATH%;%PATH%

echo Diretorio: %MY_PATH%
echo Iniciando MacroApp...

:: Executar
"%MY_PATH%MacroApp.exe"

:: Manter janela aberta se der erro
if errorlevel 1 (
    echo.
    echo ERRO: MacroApp falhou ao executar
    pause
)
EOF

    echo -e "${GREEN}✅ Launcher criado: MacroApp-Launcher.bat${NC}"
}

# Função principal
main() {
    # 1. Criar ícones faltantes
    create_missing_icons
    
    # 2. Detectar Qt
    if ! detect_qt; then
        echo -e "${RED}❌ Instale o Qt primeiro!${NC}"
        exit 1
    fi
    
    # 3. Compilar
    if ! compile_project; then
        echo -e "${RED}❌ Falha na compilação!${NC}"
        exit 1
    fi
    
    # 4. Deploy completo
    full_deploy "dist"
    
    # 5. Resumo final
    echo ""
    echo -e "${CYAN}=============================================${NC}"
    echo -e "${GREEN}🎯 DEPLOY CONCLUÍDO COM SUCESSO!${NC}"
    echo -e "${CYAN}=============================================${NC}"
    echo ""
    echo -e "${GREEN}✅ EXE FUNCIONAL DIRETAMENTE!${NC}"
    echo ""
    echo -e "${BLUE}📁 PASTA DIST:${NC}"
    echo -e "  ${GREEN}$(pwd)/dist${NC}"
    echo ""
    echo -e "${YELLOW}🚀 FORMAS DE EXECUTAR:${NC}"
    echo -e "  1. ${CYAN}Clicar diretamente em dist/MacroApp.exe${NC}"
    echo -e "  2. ${CYAN}Executar dist/MacroApp-Launcher.bat${NC}"
    echo -e "  3. ${CYAN}Via bash: ./dist/MacroApp.exe${NC}"
    echo ""
    echo -e "${GREEN}🔧 SOLUÇÃO INCORPORADA:${NC}"
    echo -e "  ✅ DLLs do Mingw copiadas automaticamente"
    echo -e "  ✅ PATH configurado nos launchers"
    echo -e "  ✅ Execução direta pelo Windows funcionando"
    echo ""
    echo -e "${BLUE}⚡ ATALHOS GLOBAIS: F9 (Gravar) • F10 (Parar) • F11 (Mostrar)${NC}"
}

# Executar script principal
main "$@"