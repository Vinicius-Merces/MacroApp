
# 🚀 MacroApp - Gravador de Macros Multi-Monitor

<div align="center">

![Qt](https://img.shields.io/badge/Qt-5.15%2B-green?logo=qt)
![Windows](https://img.shields.io/badge/Windows-10%2B-blue?logo=windows)
![C++](https://img.shields.io/badge/C++-17-red?logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-yellow)

</div>

## ✨ Características Principais

- 🎯 **Gravação Multi-Monitor** - Suporte preciso a múltiplos monitores
- ⌨️ **Gravação de Teclado e Mouse** - Captura todos os eventos de input
- 🌐 **Atalhos Globais** - F9 (Gravar), F10 (Parar), F11 (Mostrar/Ocultar)
- ⚡ **Reprodução com Humanização** - Delay variável para parecer humano
- 🎨 **Interface Moderna** - Design escuro e intuitivo
- 📦 **Sistema de Bandeja** - Execução em segundo plano
- 🔧 **Deploy Automatizado** - Script que resolve todas as DLLs

## 🛠️ Requisitos do Sistema

- **Sistema Operacional**: Windows 10 ou superior
- **Qt**: Versão 5.15 ou superior (ou Qt 6)
- **Compilador**: Compatível com C++17 (MinGW recomendado)
- **Memória**: 4GB RAM mínimo

## 🚀 Deploy Rápido

### Método 1: Script Automatizado (Recomendado)
```bash
# Execute o script de deploy
./scripts/deploy.sh

# O script irá criar uma pasta 'dist/' com:
# ✅ MacroApp.exe (executável)
# ✅ Todas as DLLs necessárias
# ✅ Launcher automático (MacroApp-Launcher.bat)
# ✅ Ícones e recursos
