#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <algorithm>  // Para std::max e std::min
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QInputDialog>
#include <QDebug>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QPalette>
#include <QCloseEvent>
#include <QPainter>
#include <QFile>
#include <map>
#include <random>
#include <thread>
#include <chrono>

MainWindow* MainWindow::instance = nullptr;

std::map<WORD, std::string> keyMap = {
    {0x41, "A"}, {0x42, "B"}, {0x43, "C"}, {0x44, "D"}, {0x45, "E"}, {0x46, "F"}, {0x47, "G"}, {0x48, "H"},
    {0x49, "I"}, {0x4A, "J"}, {0x4B, "K"}, {0x4C, "L"}, {0x4D, "M"}, {0x4E, "N"}, {0x4F, "O"}, {0x50, "P"},
    {0x51, "Q"}, {0x52, "R"}, {0x53, "S"}, {0x54, "T"}, {0x55, "U"}, {0x56, "V"}, {0x57, "W"}, {0x58, "X"},
    {0x59, "Y"}, {0x5A, "Z"}, {0x30, "0"}, {0x31, "1"}, {0x32, "2"}, {0x33, "3"}, {0x34, "4"}, {0x35, "5"},
    {0x36, "6"}, {0x37, "7"}, {0x38, "8"}, {0x39, "9"}, {VK_SPACE, "SPACE"}, {VK_RETURN, "ENTER"}, {VK_ESCAPE, "ESC"},
    {VK_SHIFT, "SHIFT"}, {VK_CONTROL, "CTRL"}, {VK_MENU, "ALT"}, {VK_TAB, "TAB"}, {VK_BACK, "BACKSPACE"},
    {VK_UP, "UP"}, {VK_DOWN, "DOWN"}, {VK_LEFT, "LEFT"}, {VK_RIGHT, "RIGHT"}, {VK_F1, "F1"}, {VK_F2, "F2"},
    {VK_F3, "F3"}, {VK_F4, "F4"}, {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"}, {VK_F9, "F9"},
    {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}
};

// CORREÇÃO COMPLETA: Callback para enumeração de monitores
BOOL CALLBACK MainWindow::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    Q_UNUSED(hdcMonitor)
    
    std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
    
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);
    
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        MonitorInfo info;
        info.index = monitors->size();
        info.left = monitorInfo.rcMonitor.left;
        info.top = monitorInfo.rcMonitor.top;
        info.right = monitorInfo.rcMonitor.right;
        info.bottom = monitorInfo.rcMonitor.bottom;
        info.rect = {info.left, info.top, info.right, info.bottom};
        info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
        
        monitors->push_back(info);
    }
    return TRUE;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    instance = this;
    
    // CORREÇÃO: Detectar monitores ANTES de qualquer operação
    DetectMonitors();
    /*
    // 🔧 BOTÃO DE TESTE VISÍVEL - SEM FALHAS
    QPushButton *testButton = new QPushButton("🧪 TESTAR PRECISÃO", this);
    testButton->setGeometry(10, 400, 200, 40); // Posição fixa na janela
    testButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff6b6b;"
        "    color: white;"
        "    border: 2px solid #ff5252;"
        "    border-radius: 8px;"
        "    padding: 8px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff5252;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #ff3838;"
        "}"
    );
    testButton->setToolTip("Clique para testar a precisão das coordenadas multi-monitor");
    connect(testButton, &QPushButton::clicked, this, &MainWindow::TestPrecision);
    */

    //TestPrecision();
    
    // Aplicar estilo moderno
    applyModernStyle();
    
    // Configurar interface
    ui->stopButton->setEnabled(false);
    ui->humanizeCheckbox->setChecked(true);
    
    // Configurar atalhos e tray
    setupShortcuts();
    setupTrayIcon();
    RegisterGlobalShortcuts();
    
    connect(ui->actionList, &QListWidget::itemDoubleClicked, this, &MainWindow::on_actionList_itemDoubleClicked);
    
    lastActionTime = std::chrono::steady_clock::now();
    
    // Mostrar mensagem de boas-vindas
    showNotification(
        "MacroApp Iniciado", 
        QString("Sistema multi-monitor detectado: %1 telas\n\nAtalhos globais:\n• F9 = Gravar\n• F10 = Parar\n• F11 = Mostrar/Ocultar")
        .arg(monitors.size()),
        false
    );
}

MainWindow::~MainWindow() {
    StopRecording();
    UnregisterGlobalShortcuts();
    delete ui;
}

void MainWindow::DetectMonitors() {
    monitors.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    
    qDebug() << "=== DETECÇÃO DE MONITORES PRECISA ===";
    qDebug() << "Monitores detectados:" << monitors.size();
    
    for (const auto& monitor : monitors) {
        qDebug() << "Monitor" << monitor.index << ":" << monitor.width << "x" << monitor.height 
                 << "at" << monitor.left << "," << monitor.top 
                 << "->" << monitor.right << "," << monitor.bottom
                 << (monitor.isPrimary ? "(PRIMÁRIO)" : "(SECUNDÁRIO)");
        
        // Verificar se as dimensões estão corretas
        int calculatedWidth = monitor.right - monitor.left;
        int calculatedHeight = monitor.bottom - monitor.top;
        
        if (monitor.width != calculatedWidth || monitor.height != calculatedHeight) {
            qDebug() << "⚠️  CORREÇÃO: Dimensões inconsistentes!";
            qDebug() << "   Esperado:" << calculatedWidth << "x" << calculatedHeight;
            qDebug() << "   Obtido:" << monitor.width << "x" << monitor.height;
        }
    }
    qDebug() << "=====================================";
}

int MainWindow::GetMonitorFromPoint(int x, int y) {
    //POINT pt = {x, y};
    
    // CORREÇÃO: Usar detecção mais precisa
    for (int i = 0; i < (int)monitors.size(); i++) {
        const auto& monitor = monitors[i];
        if (x >= monitor.left && x <= monitor.right && 
            y >= monitor.top && y <= monitor.bottom) {
            qDebug() << "Ponto (" << x << "," << y << ") detectado no monitor" << i;
            return i;
        }
    }
    
    qDebug() << "Ponto (" << x << "," << y << ") não encontrado em nenhum monitor, usando primário";
    return 0; // Fallback para monitor primário
}

std::pair<int, int> MainWindow::AbsoluteToRelative(int absX, int absY, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) {
        monitorIndex = 0;
    }
    
    const auto& monitor = monitors[monitorIndex];
    
    // CORREÇÃO: Cálculo mais preciso usando double para evitar perda de precisão
    double width = static_cast<double>(monitor.width);
    double height = static_cast<double>(monitor.height);
    
    int relX = static_cast<int>(((absX - monitor.left) * 10000.0) / width);
    int relY = static_cast<int>(((absY - monitor.top) * 10000.0) / height);
    
    // Garantir que esteja dentro dos limites
    relX = std::max(0, std::min(10000, relX));
    relY = std::max(0, std::min(10000, relY));
    
    qDebug() << "AbsToRel - Monitor" << monitorIndex << ":" 
             << "Abs(" << absX << "," << absY << ")" 
             << "-> Rel(" << relX << "," << relY << ")"
             << "Dimensões:" << monitor.width << "x" << monitor.height;
    
    return std::make_pair(relX, relY);
}

std::pair<int, int> MainWindow::RelativeToAbsolute(int relX, int relY, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) {
        qDebug() << "⚠️  Índice de monitor inválido em RelativeToAbsolute:" << monitorIndex;
        monitorIndex = 0;
    }
    
    const auto& monitor = monitors[monitorIndex];
    
    // 🔧 CORREÇÃO: Usar cálculo mais preciso com double
    double width = static_cast<double>(monitor.width);
    double height = static_cast<double>(monitor.height);
    
    // Converter porcentagem (0-10000) para coordenadas absolutas
    int absX = monitor.left + static_cast<int>((relX * width) / 10000.0);
    int absY = monitor.top + static_cast<int>((relY * height) / 10000.0);
    
    // 🔧 CORREÇÃO: Garantir que está dentro dos limites VISÍVEIS do monitor
    absX = std::max(monitor.left, std::min(monitor.right - 1, absX));
    absY = std::max(monitor.top, std::min(monitor.bottom - 1, absY));
    
    qDebug() << "RelToAbs - Monitor" << monitorIndex << ":" 
             << "Rel(" << relX << "," << relY << ")" 
             << "-> Abs(" << absX << "," << absY << ")"
             << "Dimensões:" << monitor.width << "x" << monitor.height
             << "Área:" << monitor.left << "," << monitor.top 
             << "->" << monitor.right << "," << monitor.bottom;
    
    return std::make_pair(absX, absY);
}
void MainWindow::applyModernStyle() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(45, 45, 48));
    palette.setColor(QPalette::WindowText, QColor(240, 240, 240));
    palette.setColor(QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    palette.setColor(QPalette::ToolTipBase, QColor(240, 240, 240));
    palette.setColor(QPalette::ToolTipText, QColor(240, 240, 240));
    palette.setColor(QPalette::Text, QColor(240, 240, 240));
    palette.setColor(QPalette::Button, QColor(63, 63, 70));
    palette.setColor(QPalette::ButtonText, QColor(240, 240, 240));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    
    qApp->setPalette(palette);
    
    QString buttonStyle = 
        "QPushButton {"
        "    background-color: #3f3f46;"
        "    border: 2px solid #565656;"
        "    border-radius: 8px;"
        "    padding: 8px 16px;"
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #52525b;"
        "    border-color: #71717a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2a82da;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #27272a;"
        "    color: #71717a;"
        "}";
    
    qApp->setStyleSheet(buttonStyle + 
        "QMainWindow { background-color: #2d2d30; }"
        "QListWidget {"
        "    background-color: #1e1e1e;"
        "    border: 2px solid #565656;"
        "    border-radius: 8px;"
        "    color: white;"
        "    font-family: 'Segoe UI', Arial;"
        "}"
        "QListWidget::item {"
        "    padding: 6px;"
        "    border-bottom: 1px solid #3f3f46;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #2a82da;"
        "}"
        "QLineEdit {"
        "    background-color: #1e1e1e;"
        "    border: 2px solid #565656;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    color: white;"
        "    selection-background-color: #2a82da;"
        "}"
        "QCheckBox {"
        "    color: white;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    border: 2px solid #565656;"
        "    background-color: #1e1e1e;"
        "    border-radius: 4px;"
        "}"
        "QCheckBox::indicator:checked {"
        "    border: 2px solid #2a82da;"
        "    background-color: #2a82da;"
        "    border-radius: 4px;"
        "}"
        "QLabel {"
        "    color: white;"
        "    font-weight: bold;"
        "}");
}

void MainWindow::setupShortcuts() {
    // Atalhos locais (apenas quando a janela está ativa)
    QShortcut *startShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(startShortcut, &QShortcut::activated, this, &MainWindow::startRecordingShortcut);
    
    QShortcut *stopShortcut = new QShortcut(QKeySequence("Ctrl+P"), this);
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stopRecordingShortcut);
}

void MainWindow::setupTrayIcon() {
    // Criar o objeto QSystemTrayIcon
    trayIcon = new QSystemTrayIcon(this);
    
    // Carregar ícone das suas imagens
    QIcon trayIconIcon;
    if (QFile::exists(":/icons/app_icon.png")) {
        trayIconIcon = QIcon(":/icons/app_icon.png");
        qDebug() << "Usando app_icon.png para bandeja";
    } else if (QFile::exists(":/icons/app_icon.ico")) {
        trayIconIcon = QIcon(":/icons/app_icon.ico");
        qDebug() << "Usando app_icon.ico para bandeja";
    } else {
        // Fallback: criar ícone programático
        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(42, 130, 218));
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "M");
        trayIconIcon = QIcon(pixmap);
        qDebug() << "Usando ícone programático para bandeja";
    }
    
    trayIcon->setIcon(trayIconIcon);
    trayIcon->setToolTip("MacroApp - Gravador de Macros\nF9: Gravar | F10: Parar | F11: Mostrar");
    
    trayMenu = new QMenu(this);
    
    // Adicionar logo ao menu se existir
    if (QFile::exists(":/icons/logo.png")) {
        QAction *logoAction = new QAction(this);
        logoAction->setIcon(QIcon(":/icons/logo.png"));
        logoAction->setText("MacroApp v1.0");
        logoAction->setEnabled(false);
        trayMenu->addAction(logoAction);
        trayMenu->addSeparator();
        qDebug() << "Logo adicionado ao menu da bandeja";
    }
    
    QAction *showAction = new QAction("🎯 Mostrar Janela", this);
    QAction *hideAction = new QAction("⬇️ Ocultar Janela", this);
    QAction *quitAction = new QAction("❌ Sair", this);
    
    connect(showAction, &QAction::triggered, this, &MainWindow::showWindow);
    connect(hideAction, &QAction::triggered, this, &MainWindow::hideWindow);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    trayMenu->addAction(showAction);
    trayMenu->addAction(hideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
    
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::on_trayIcon_activated);
    
    qDebug() << "Ícone da bandeja configurado com sucesso";
}

void MainWindow::showNotification(const QString &title, const QString &message, bool isWarning) {
    if (trayIcon && trayIcon->isVisible()) {
        QSystemTrayIcon::MessageIcon icon = isWarning ? 
            QSystemTrayIcon::Warning : QSystemTrayIcon::Information;
        trayIcon->showMessage(title, message, icon, 3000);
    } else {
        if (isWarning) {
            QMessageBox::warning(this, title, message);
        } else {
            QMessageBox::information(this, title, message);
        }
    }
}

// Hook para atalhos globais (F9, F10, F11)
LRESULT CALLBACK MainWindow::GlobalShortcutHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && MainWindow::instance) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        
        if (isKeyDown) {
            // F9 para iniciar gravação
            if (kbStruct->vkCode == VK_F9) {
                MainWindow::instance->HandleGlobalShortcut(VK_F9);
                return 1;
            }
            // F10 para parar gravação
            else if (kbStruct->vkCode == VK_F10) {
                MainWindow::instance->HandleGlobalShortcut(VK_F10);
                return 1;
            }
            // F11 para mostrar/ocultar
            else if (kbStruct->vkCode == VK_F11) {
                if (MainWindow::instance->isHidden()) {
                    MainWindow::instance->showWindow();
                } else {
                    MainWindow::instance->hideWindow();
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void MainWindow::HandleGlobalShortcut(WORD vkCode) {
    if (vkCode == VK_F9 && !isRecording) {
        StartRecording();
    }
    else if (vkCode == VK_F10 && isRecording) {
        StopRecording();
    }
}

void MainWindow::RegisterGlobalShortcuts() {
    globalShortcutHook = SetWindowsHookEx(WH_KEYBOARD_LL, GlobalShortcutHookProc, GetModuleHandle(NULL), 0);
    if (!globalShortcutHook) {
        showNotification("Aviso", "Não foi possível registrar atalhos globais. Use os botões da interface.", true);
    }
}

void MainWindow::UnregisterGlobalShortcuts() {
    if (globalShortcutHook) {
        UnhookWindowsHookEx(globalShortcutHook);
        globalShortcutHook = nullptr;
    }
}

// Hooks para gravação
LRESULT CALLBACK MainWindow::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && MainWindow::instance && MainWindow::instance->isRecording && MainWindow::instance->recordingKeyboard) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        
        // Ignorar atalhos globais durante gravação
        bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        if (ctrlPressed && altPressed && (kbStruct->vkCode == 'S' || kbStruct->vkCode == 'P')) {
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        
        MainWindow::instance->RecordKeyEvent(kbStruct->vkCode, isKeyDown);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MainWindow::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && MainWindow::instance && MainWindow::instance->isRecording && MainWindow::instance->recordingMouse) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        
        switch (wParam) {
            case WM_LBUTTONDOWN:
                MainWindow::instance->RecordMouseEvent(mouseStruct->pt.x, mouseStruct->pt.y, 0, true);
                break;
            case WM_LBUTTONUP:
                MainWindow::instance->RecordMouseEvent(mouseStruct->pt.x, mouseStruct->pt.y, 0, false);
                break;
            case WM_RBUTTONDOWN:
                MainWindow::instance->RecordMouseEvent(mouseStruct->pt.x, mouseStruct->pt.y, 1, true);
                break;
            case WM_RBUTTONUP:
                MainWindow::instance->RecordMouseEvent(mouseStruct->pt.x, mouseStruct->pt.y, 1, false);
                break;
            case WM_MOUSEMOVE:
                MainWindow::instance->RecordMouseMove(mouseStruct->pt.x, mouseStruct->pt.y);
                break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void MainWindow::StartRecording() {
    // CORREÇÃO: Atualizar detecção de monitores ANTES de gravar
    DetectMonitors();
    
    // Instalar hooks de gravação
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    
    if (keyboardHook && mouseHook) {
        isRecording = true;
        recordingKeyboard = ui->recordKeyboardCheckbox->isChecked();
        recordingMouse = ui->recordMouseCheckbox->isChecked();
        recorded_actions.clear();
        ui->actionList->clear();
        lastActionTime = std::chrono::steady_clock::now();
        
        ui->recordButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        
        showNotification(
            "🎤 Gravação Iniciada", 
            QString("Gravando em %1 monitor(es)\nTeclado: %2 | Mouse: %3\n\nPressione F10 para parar")
            .arg(monitors.size())
            .arg(recordingKeyboard ? "✓" : "✗")
            .arg(recordingMouse ? "✓" : "✗"),
            false
        );
    } else {
        showNotification("Erro", "Não foi possível iniciar a gravação.", true);
    }
}

void MainWindow::StopRecording() {
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = nullptr;
    }
    
    isRecording = false;
    ui->recordButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    
    UpdateActionList();
    
    showNotification(
        "⏹️ Gravação Finalizada", 
        QString("%1 ações gravadas com sucesso!\n\nCliques registrados em %2 monitor(es) diferentes")
        .arg(recorded_actions.size())
        .arg(monitors.size()),
        false
    );
}

void MainWindow::RecordKeyEvent(WORD vkCode, bool isKeyDown) {
    auto now = std::chrono::steady_clock::now();
    double delay = std::chrono::duration<double>(now - lastActionTime).count();
    lastActionTime = now;
    
    if (delay > 0.01) {
        Action delayAction = {"delay", 0, 0, 0, false, delay, 0, -1};
        recorded_actions.push_back(delayAction);
    }
    
    Action keyAction = {"key_press", 0, 0, vkCode, isKeyDown, 0.0, 0, -1};
    recorded_actions.push_back(keyAction);
    
    UpdateActionList();
}

void MainWindow::RecordMouseEvent(int x, int y, int button, bool isButtonDown) {
    auto now = std::chrono::steady_clock::now();
    double delay = std::chrono::duration<double>(now - lastActionTime).count();
    lastActionTime = now;
    
    if (delay > 0.01) {
        Action delayAction = {"delay", 0, 0, 0, false, delay, 0, -1};
        recorded_actions.push_back(delayAction);
    }
    
    // CORREÇÃO: Determinar em qual monitor o evento ocorreu
    int monitorIndex = GetMonitorFromPoint(x, y);
    
    // Converter para coordenadas relativas ao monitor específico
    auto relativePos = AbsoluteToRelative(x, y, monitorIndex);
    
    Action mouseAction = {"mouse_click", relativePos.first, relativePos.second, 
                         (WORD)button, isButtonDown, 0.0, 0, monitorIndex};
    recorded_actions.push_back(mouseAction);
    
    qDebug() << "Mouse click gravado - Monitor:" << monitorIndex << "Pos:" << relativePos.first << "," << relativePos.second;
    
    UpdateActionList();
}

void MainWindow::RecordMouseMove(int x, int y) {
    static int lastX = -1, lastY = -1;
    if (lastX != -1 && lastY != -1) {
        int deltaX = abs(x - lastX);
        int deltaY = abs(y - lastY);
        
        // Gravar movimento apenas se for significativo
        if (deltaX > 5 || deltaY > 5) {
            auto now = std::chrono::steady_clock::now();
            double delay = std::chrono::duration<double>(now - lastActionTime).count();
            lastActionTime = now;
            
            if (delay > 0.01) {
                Action delayAction = {"delay", 0, 0, 0, false, delay, 0, -1};
                recorded_actions.push_back(delayAction);
            }
            
            // CORREÇÃO: Identificar monitor correto para movimento
            int monitorIndex = GetMonitorFromPoint(x, y);
            auto relativePos = AbsoluteToRelative(x, y, monitorIndex);
            
            Action moveAction = {"mouse_move", relativePos.first, relativePos.second, 0, false, 0.0, 0, monitorIndex};
            recorded_actions.push_back(moveAction);
            
            qDebug() << "Mouse move gravado - Monitor:" << monitorIndex << "Pos:" << relativePos.first << "," << relativePos.second;
            
            UpdateActionList();
        }
    }
    lastX = x;
    lastY = y;
}

std::string MainWindow::KeyCodeToString(WORD vkCode) {
    auto it = keyMap.find(vkCode);
    if (it != keyMap.end()) {
        return it->second;
    }
    return "KEY_" + std::to_string(vkCode);
}

std::string MainWindow::MouseButtonToString(int button) {
    switch (button) {
        case 0: return "LEFT";
        case 1: return "RIGHT";
        case 2: return "MIDDLE";
        default: return "BUTTON_" + std::to_string(button);
    }
}

void MainWindow::UpdateActionList() {
    ui->actionList->clear();
    
    for (size_t i = 0; i < recorded_actions.size(); ++i) {
        const auto& action = recorded_actions[i];
        QString itemText;
        
        if (action.type == "key_press") {
            QString state = action.pressed ? "DOWN" : "UP";
            itemText = QString("%1. KEY: %2 [%3]")
                .arg(i + 1)
                .arg(QString::fromStdString(KeyCodeToString(action.key)))
                .arg(state);
        }
        else if (action.type == "mouse_click") {
            QString state = action.pressed ? "DOWN" : "UP";
            QString monitorInfo = action.monitorIndex >= 0 ? 
                QString("Tela %1").arg(action.monitorIndex + 1) : "Tela ?";
            itemText = QString("%1. MOUSE: %2 [%3] at (%4%%, %5%%) [%6]")
                .arg(i + 1)
                .arg(QString::fromStdString(MouseButtonToString(action.key)))
                .arg(state)
                .arg(action.x / 100.0, 0, 'f', 1)
                .arg(action.y / 100.0, 0, 'f', 1)
                .arg(monitorInfo);
        }
        else if (action.type == "mouse_move") {
            QString monitorInfo = action.monitorIndex >= 0 ? 
                QString("Tela %1").arg(action.monitorIndex + 1) : "Tela ?";
            itemText = QString("%1. MOUSE MOVE to (%2%%, %3%%) [%4]")
                .arg(i + 1)
                .arg(action.x / 100.0, 0, 'f', 1)
                .arg(action.y / 100.0, 0, 'f', 1)
                .arg(monitorInfo);
        }
        else if (action.type == "delay") {
            itemText = QString("%1. DELAY: %2s")
                .arg(i + 1)
                .arg(action.delay, 0, 'f', 3);
        }
        
        ui->actionList->addItem(itemText);
    }
    
    if (recorded_actions.size() > 0) {
        ui->actionList->scrollToBottom();
    }
}

void MainWindow::SendKey(WORD vk, bool press) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = 0;
    input.ki.dwFlags = press ? 0 : KEYEVENTF_KEYUP;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

void MainWindow::SendMouseClick(int button, bool press) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    
    switch (button) {
        case 0: // Left
            input.mi.dwFlags = press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
            break;
        case 1: // Right
            input.mi.dwFlags = press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
            break;
        case 2: // Middle
            input.mi.dwFlags = press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
            break;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

void MainWindow::SendMouseMove(int relX, int relY, int monitorIndex) {
    // 🔧 CORREÇÃO CRÍTICA: Validar índice do monitor
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) {
        qDebug() << "❌ Índice de monitor inválido:" << monitorIndex << ", usando monitor 0";
        monitorIndex = 0;
    }
    
    // 🔧 CORREÇÃO: Remover variável não utilizada 'monitor'
    // const auto& monitor = monitors[monitorIndex]; // REMOVIDO
    
    qDebug() << "=== ENVIANDO MOVIMENTO DO MOUSE ===";
    qDebug() << "Monitor destino:" << monitorIndex;
    qDebug() << "Coordenadas relativas:" << relX << "," << relY;
    
    // 🔧 CORREÇÃO: Converter para coordenadas absolutas
    auto absPos = RelativeToAbsolute(relX, relY, monitorIndex);
    int absX = absPos.first;
    int absY = absPos.second;
    
    qDebug() << "Coordenadas absolutas calculadas:" << absX << "," << absY;
    
    // 🔧 NOVA VERIFICAÇÃO: Confirmar que está no monitor correto
    int detectedMonitor = GetMonitorFromPoint(absX, absY);
    if (detectedMonitor != monitorIndex) {
        qDebug() << "⚠️  AVISO: Ponto caiu no monitor" << detectedMonitor << "em vez de" << monitorIndex;
        // Recalcular para o monitor correto
        absPos = RelativeToAbsolute(relX, relY, detectedMonitor);
        absX = absPos.first;
        absY = absPos.second;
        qDebug() << "Coordenadas recalculadas:" << absX << "," << absY;
    }
    
    // Obter informações do desktop virtual
    int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    qDebug() << "Desktop virtual:" << virtualLeft << "," << virtualTop 
             << "->" << virtualWidth << "x" << virtualHeight;
    
    // 🔧 CORREÇÃO: Evitar divisão por zero
    int effectiveVirtualWidth = std::max(1, virtualWidth);
    int effectiveVirtualHeight = std::max(1, virtualHeight);

    double normalizedX = ((absX - virtualLeft) * 65535.0) / effectiveVirtualWidth;
    double normalizedY = ((absY - virtualTop) * 65535.0) / effectiveVirtualHeight;

    int finalX = std::max(0, std::min(65535, (int)normalizedX));
    int finalY = std::max(0, std::min(65535, (int)normalizedY));
    
    qDebug() << "Coordenadas normalizadas:" << finalX << "," << finalY;
    
    // Enviar movimento do mouse
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = finalX;
    input.mi.dy = finalY;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;
    
    if (SendInput(1, &input, sizeof(INPUT))) {
        qDebug() << "✅ Movimento do mouse enviado com sucesso!";
        
        // 🔧 CORREÇÃO: Verificar posição final real
        POINT actualPos;
        GetCursorPos(&actualPos);
        qDebug() << "Posição real do cursor:" << actualPos.x << "," << actualPos.y;
        
        int actualMonitor = GetMonitorFromPoint(actualPos.x, actualPos.y);
        qDebug() << "Monitor real:" << actualMonitor;
        
    } else {
        qDebug() << "❌ Falha ao enviar movimento do mouse!";
    }
    
    qDebug() << "=====================================";
    
    // Delay para estabilidade
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void MainWindow::TestPrecision() {
    qDebug() << "🎯 FUNÇÃO TestPrecision() CHAMADA!";
    
    // 🔧 CRIAR ARQUIVO DE LOG
    QFile logFile("precision_test.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << "=== TESTE DE PRECISÃO MACROAPP ===\n";
        out << "Data: " << QDateTime::currentDateTime().toString() << "\n\n";
        
        DetectMonitors();
        
        out << "Monitores detectados: " << monitors.size() << "\n";
        
        // 🔧 CORREÇÃO: Usar size_t para evitar warning
        for (size_t i = 0; i < monitors.size(); i++) {
            const auto& monitor = monitors[i];
            out << "\n--- Monitor " << i << (monitor.isPrimary ? " (PRIMÁRIO)" : " (SECUNDÁRIO)") << " ---\n";
            out << "Dimensões: " << monitor.width << " x " << monitor.height << "\n";
            out << "Área: " << monitor.left << ", " << monitor.top << " -> " << monitor.right << ", " << monitor.bottom << "\n";
            
            std::vector<std::pair<int, int>> testPoints = {
                {monitor.left, monitor.top},
                {monitor.right - 1, monitor.top},
                {monitor.left, monitor.bottom - 1},
                {monitor.right - 1, monitor.bottom - 1},
                {monitor.left + monitor.width/2, monitor.top + monitor.height/2}
            };
            
            out << "\nTeste de pontos:\n";
            for (const auto& point : testPoints) {
                auto rel = AbsoluteToRelative(point.first, point.second, i);
                auto abs = RelativeToAbsolute(rel.first, rel.second, i);
                
                int diffX = abs.first - point.first;
                int diffY = abs.second - point.second;
                
                out << "  Ponto: " << point.first << ", " << point.second 
                    << " -> Rel: " << rel.first << ", " << rel.second
                    << " -> Abs: " << abs.first << ", " << abs.second
                    << " | Dif: " << diffX << ", " << diffY;
                
                // 🔧 CORREÇÃO CRÍTICA: Usar std::abs explicitamente
                if (std::abs(diffX) > 5 || std::abs(diffY) > 5) {
                    out << " ❌ ERRO GRANDE!";
                } else if (std::abs(diffX) > 1 || std::abs(diffY) > 1) {
                    out << " ⚠️  Pequeno erro";
                } else {
                    out << " ✅ OK";
                }
                out << "\n";
            }
        }
        
        out << "\n=== FIM DO TESTE ===\n";
        logFile.close();
        
        // 🔧 MOSTRAR MENSAGEM NA INTERFACE
        showNotification("Teste Concluído", 
            QString("Teste de precisão salvo em:\n%1\n\nVerifique o arquivo precision_test.log")
            .arg(QDir::current().absoluteFilePath("precision_test.log")), 
            false);
            
        qDebug() << "📄 Log salvo em:" << QDir::current().absoluteFilePath("precision_test.log");
        
    } else {
        showNotification("Erro", "Não foi possível criar arquivo de log.", true);
    }
}

void MainWindow::on_playButton_clicked() {
    if (recorded_actions.empty()) {
        showNotification("Aviso", "Nenhuma ação para reproduzir.", true);
        return;
    }
    
    // 🔧 CORREÇÃO 1: Adicionar esta linha - detectar monitores antes de reproduzir
    DetectMonitors();
    
    bool ok;
    int reps = ui->repsEdit->text().toInt(&ok);
    if (!ok || reps <= 0) reps = 1;
    
    double var_max = ui->varMaxEdit->text().toDouble(&ok);
    if (!ok) var_max = 0.1;
    
    bool humanize = ui->humanizeCheckbox->isChecked();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-var_max, var_max);
    
    showNotification(
        "▶️ Reprodução Iniciada", 
        QString("Reproduzindo %1 vezes%2\nAções em %3 monitor(es) diferentes")
        .arg(reps)
        .arg(humanize ? " com humanização" : "")
        .arg(monitors.size()),
        false
    );
    
    // Pequeno delay antes de começar
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (monitors.empty()) {
    showNotification("Erro", "Nenhum monitor detectado. Não é possível reproduzir ações de mouse.", true);
    return;
    }
    
    for (int rep = 0; rep < reps; ++rep) {
        for (const auto& action : recorded_actions) {
            if (action.type == "key_press") {
                SendKey(action.key, action.pressed);
            } 
            else if (action.type == "mouse_click") {
                // 🔧 CORREÇÃO: Movimento + Delay aumentado + Clique
                qDebug() << "🎯 Preparando clique do mouse...";
                SendMouseMove(action.x, action.y, action.monitorIndex);
                
                // 🔧 AUMENTAR DELAY para garantir estabilidade
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                
                // Verificar posição atual do cursor
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                qDebug() << "Posição do cursor antes do clique:" << cursorPos.x << "," << cursorPos.y;
                
                SendMouseClick(action.key, action.pressed);
                
                // Pequeno delay após o clique
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            else if (action.type == "mouse_move") {
                SendMouseMove(action.x, action.y, action.monitorIndex);
            }
            
            if (action.delay > 0) {
                double actualDelay = action.delay;
                if (humanize) {
                    double variation = dis(gen);
                    actualDelay += variation;
                    actualDelay = std::max(0.01, actualDelay);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(actualDelay * 1000)));
            }
        }
        
        if (rep < reps - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    showNotification("✅ Reprodução Concluída", "Todas as ações foram executadas com sucesso!", false);
}

void MainWindow::on_recordButton_clicked() {
    StartRecording();
}

void MainWindow::on_stopButton_clicked() {
    StopRecording();
}

void MainWindow::on_saveButton_clicked() {
    if (recorded_actions.empty()) {
        showNotification("Aviso", "Nenhuma ação para salvar.", true);
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Salvar Macro", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonArray array;
            for (const auto& act : recorded_actions) {
                QJsonObject obj;
                obj["type"] = QString::fromStdString(act.type);
                obj["x"] = act.x;
                obj["y"] = act.y;
                obj["key"] = (int)act.key;
                obj["pressed"] = act.pressed;
                obj["delay"] = act.delay;
                obj["frequency"] = act.frequency;
                obj["monitorIndex"] = act.monitorIndex;
                array.append(obj);
            }
            file.write(QJsonDocument(array).toJson());
            file.close();
            showNotification("💾 Macro Salvo", "Arquivo salvo com sucesso!", false);
        } else {
            showNotification("Erro", "Não foi possível salvar o arquivo.", true);
        }
    }
}

void MainWindow::on_loadButton_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Carregar Macro", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isNull()) {
                showNotification("Erro", "Arquivo JSON inválido.", true);
                return;
            }
            
            recorded_actions.clear();
            QJsonArray array = doc.array();
            for (const auto& obj : array) {
                QJsonObject o = obj.toObject();
                Action act = {
                    o["type"].toString().toStdString(),
                    o["x"].toInt(),
                    o["y"].toInt(),
                    (WORD)o["key"].toInt(),
                    o["pressed"].toBool(),
                    o["delay"].toDouble(),
                    o["frequency"].toInt(),
                    o["monitorIndex"].toInt(-1) // -1 para arquivos antigos
                };
                recorded_actions.push_back(act);
            }
            file.close();
            UpdateActionList();
            showNotification(
                "📂 Macro Carregado", 
                QString("%1 ações carregadas com sucesso!").arg(recorded_actions.size()),
                false
            );
        } else {
            showNotification("Erro", "Não foi possível abrir o arquivo.", true);
        }
    }
}

void MainWindow::on_clearButton_clicked() {
    if (QMessageBox::question(this, "Limpar", "Tem certeza que deseja limpar todas as ações?") == QMessageBox::Yes) {
        recorded_actions.clear();
        ui->actionList->clear();
        showNotification("🗑️ Ações Limpas", "Todas as ações foram removidas.", false);
    }
}

void MainWindow::on_actionList_itemDoubleClicked(QListWidgetItem *item) {
    int index = ui->actionList->row(item);
    if (index >= 0 && index < (int)recorded_actions.size()) {
        Action& action = recorded_actions[index];
        
        if (action.type == "delay") {
            bool ok;
            double newDelay = QInputDialog::getDouble(this, "Editar Delay", 
                "Novo delay (segundos):", action.delay, 0.0, 10.0, 3, &ok);
            if (ok) {
                action.delay = newDelay;
                UpdateActionList();
            }
        }
    }
}

void MainWindow::on_humanizeCheckbox_stateChanged(int state) {
    ui->varMaxEdit->setEnabled(state == Qt::Checked);
    ui->varMaxLabel->setEnabled(state == Qt::Checked);
}

void MainWindow::on_trayIcon_activated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isHidden()) {
            showWindow();
        } else {
            hideWindow();
        }
    }
}

void MainWindow::showWindow() {
    show();
    raise();
    activateWindow();
}

void MainWindow::hideWindow() {
    hide();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon->isVisible()) {
        hide();
        showNotification(
            "MacroApp", 
            "Aplicação minimizada para bandeja\n\nAtalhos globais:\n• F9 = Gravar\n• F10 = Parar\n• F11 = Mostrar/Ocultar",
            false
        );
        event->ignore();
    }
}

void MainWindow::startRecordingShortcut() {
    if (!isRecording) {
        StartRecording();
    }
}

void MainWindow::stopRecordingShortcut() {
    if (isRecording) {
        StopRecording();
    }
}