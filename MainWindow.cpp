#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include <QtWidgets/QtWidgets>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <vector>
#include <string>
#include <regex>
#include <functional>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
    ui->setupUi(this);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_do_file_clicked(){
    const auto value = QFileDialog::getOpenFileName();
    if( value.isEmpty() ){return;}
    ui->path_file->setText(value);
}

void MainWindow::on_do_work_clicked(){
    const auto value = QFileDialog::getExistingDirectory();
    if( value.isEmpty() ){return;}
    ui->path_work->setText(value);
}

void MainWindow::on_do_work_target_clicked(){
    const auto value = QFileDialog::getExistingDirectory();
    if( value.isEmpty() ){return;}
    ui->path_work_target->setText(value);
}

void MainWindow::on_copy_clicked(){
    const std::regex * varR = nullptr ;
    {/** 构建正则表达式 **/
        QFile varFile{ ui->path_file->text() };
        varFile.open( QIODevice::ReadOnly );
        QTextStream varStream{ &varFile };
        std::string varRegText;
        while ( varStream.atEnd() == false ) {
            const QString varLine = varStream.readLine().trimmed() ;
            if( varLine.isEmpty() ){ continue; }
            varRegText += u8'(';
            varRegText += varLine.toUtf8().toStdString()  ;
            varRegText += u8')';
            varRegText += u8'|';
        }
        varRegText.pop_back();
        varRegText.shrink_to_fit();
        try{
            const std::regex * globalR = new std::regex { varRegText };
            varR = globalR;
        }catch( const std::regex_error & e ){
            ui->errorShow->setPlainText( QString::fromLocal8Bit( e.what() ) );
            return;
        }
    }

    /*获得所有文件路径*/

    /*执行拷贝*/

}



