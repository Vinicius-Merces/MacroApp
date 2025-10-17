#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QFile>
#include <windows.h>
#include <vector>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct MonitorInfo {
    int index;
    int left;
    int top;
    int right;
    int bottom;
    int width;
    int height;
    bool isPrimary;
    RECT rect;
};

struct Action {
    std::string type;
    int x;
    int y;
    WORD key;
    bool pressed;
    double delay;
    int frequency;
    int monitorIndex;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow* instance;
    
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Funções de monitor
    void DetectMonitors();
    int GetMonitorFromPoint(int x, int y);
    std::pair<int, int> AbsoluteToRelative(int absX, int absY, int monitorIndex);
    std::pair<int, int> RelativeToAbsolute(int relX, int relY, int monitorIndex);

    // Funções de gravação e reprodução
    void StartRecording();
    void StopRecording();
    void RecordKeyEvent(WORD vkCode, bool isKeyDown);
    void RecordMouseEvent(int x, int y, int button, bool isButtonDown);
    void RecordMouseMove(int x, int y);
    
    // Funções de input
    void SendKey(WORD vk, bool press);
    void SendMouseClick(int button, bool press);
    void SendMouseMove(int relX, int relY, int monitorIndex);
    
    // Utilitários
    std::string KeyCodeToString(WORD vkCode);
    std::string MouseButtonToString(int button);
    void UpdateActionList();
    void showNotification(const QString &title, const QString &message, bool isWarning = false);
    
    // Funções de interface
    void showWindow();
    void hideWindow();
    void startRecordingShortcut();
    void stopRecordingShortcut();
    

private slots:
    void on_playButton_clicked();
    void on_recordButton_clicked();
    void on_stopButton_clicked();
    void on_saveButton_clicked();
    void on_loadButton_clicked();
    void on_clearButton_clicked();
    void on_actionList_itemDoubleClicked(QListWidgetItem *item);
    void on_humanizeCheckbox_stateChanged(int state);
    void on_trayIcon_activated(QSystemTrayIcon::ActivationReason reason);
    void TestPrecision(); // ✅ ADICIONAR ESTA LINHA

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    
    // Variáveis de estado
    bool isRecording = false;
    bool recordingKeyboard = true;
    bool recordingMouse = true;
    std::chrono::steady_clock::time_point lastActionTime;
    
    // Hooks
    HHOOK keyboardHook = nullptr;
    HHOOK mouseHook = nullptr;
    HHOOK globalShortcutHook = nullptr;
    
    // Dados
    std::vector<Action> recorded_actions;
    std::vector<MonitorInfo> monitors;
    
    // Tray
    QSystemTrayIcon *trayIcon = nullptr;
    QMenu *trayMenu = nullptr;
    
    // Funções privadas
    void applyModernStyle();
    void setupShortcuts();
    void setupTrayIcon();
    void RegisterGlobalShortcuts();
    void UnregisterGlobalShortcuts();
    void HandleGlobalShortcut(WORD vkCode);
    
    // Hooks estáticos
    static LRESULT CALLBACK GlobalShortcutHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // Função de callback para enumeração de monitores
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
};

#endif // MAINWINDOW_H