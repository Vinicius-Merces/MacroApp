#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    
    // Definir informações da aplicação
    a.setApplicationName("MacroApp");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("MacroApp");
    
    // Debug: verificar se os recursos estão carregados
    qDebug() << "Verificando recursos...";
    qDebug() << "Ícone ICO existe:" << QFile::exists(":/icons/app_icon.ico");
    qDebug() << "Ícone PNG existe:" << QFile::exists(":/icons/app_icon.png");
    qDebug() << "Logo existe:" << QFile::exists(":/icons/logo.png");
    
    // Carregar ícone principal
    QIcon appIcon;
    if (QFile::exists(":/icons/app_icon.ico")) {
        appIcon = QIcon(":/icons/app_icon.ico");
        qDebug() << "Usando ícone ICO";
    } else if (QFile::exists(":/icons/app_icon.png")) {
        appIcon = QIcon(":/icons/app_icon.png");
        qDebug() << "Usando ícone PNG";
    } else {
        // Fallback para ícone do sistema
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        qDebug() << "Usando ícone padrão do sistema";
    }
    
    a.setWindowIcon(appIcon);
    
    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Erro Fatal", 
                            QString("Erro ao iniciar a aplicação:\n%1").arg(e.what()));
        return -1;
    }
}