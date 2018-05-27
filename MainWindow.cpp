#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include <QtWidgets/QtWidgets>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <vector>
#include <string>
#include <regex>
#include <functional>
#include <filesystem>
#include <tuple>
#include <execution>
#include <set>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow) {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::on_do_file_clicked() {
	const auto value = QFileDialog::getOpenFileName();
	if (value.isEmpty()) { return; }
	ui->path_file->setText(value);
}

void MainWindow::on_do_work_clicked() {
	const auto value = QFileDialog::getExistingDirectory();
	if (value.isEmpty()) { return; }
	ui->path_work->setText(value);
}

void MainWindow::on_do_work_target_clicked() {
	const auto value = QFileDialog::getExistingDirectory();
	if (value.isEmpty()) { return; }
	ui->path_work_target->setText(value);
}

void MainWindow::on_copy_clicked() try {
	std::shared_ptr< const std::regex >  varR = nullptr;
	{/** 构建正则表达式 **/
		QFile varFile{ ui->path_file->text() };
		varFile.open(QIODevice::ReadOnly);
		QTextStream varStream{ &varFile };
		std::string varRegText;
		std::set<QString> varUnique;
		while (varStream.atEnd() == false) {
			const QString varLine = varStream.readLine().trimmed().toLower();
			if (varLine.isEmpty()) { continue; }
			if (varUnique.insert(varLine).second == false) {
				continue;
			}
			varRegText += u8'(';
			varRegText += varLine.toUtf8().toStdString();
			varRegText += u8')';
			varRegText += u8'|';
		}
		varRegText.pop_back();
		varRegText.shrink_to_fit();
		try {
			const std::regex * globalR = new std::regex{ varRegText ,
			std::regex::icase };
			varR.reset(globalR);
		}
		catch (const std::regex_error & e) {
			ui->errorShow->setPlainText(QString::fromLocal8Bit(e.what()));
			return;
		}
	}

	/*获得所有文件路径*/
	namespace fs = std::filesystem;
	fs::path varRootDirPath;
	std::vector< std::tuple< fs::path, fs::path > > varPaths;
	{
		{
			QDir varRootDirPathQ{ ui->path_work->text() };
			varRootDirPath =
				fs::u8path(varRootDirPathQ.absolutePath().toUtf8().toStdString());
		}

		fs::recursive_directory_iterator varIt{ varRootDirPath ,
			fs::directory_options::skip_permission_denied &
			(~fs::directory_options::follow_directory_symlink) };
		for (auto & varP : varIt) {
			if (varP.is_directory())continue;
			auto varPathR = fs::relative(varP, varRootDirPath);
			const auto varJudge = varPathR.u8string();
			if (false == std::regex_search(varJudge, *varR))
				continue;
			varPaths.emplace_back(std::move(varPathR), std::move(varP));
		}
	}

	/*执行拷贝*/
	fs::path varTargetDirPath;
	std::vector< std::function<void(void)> > dutys;
	dutys.reserve(varPaths.size());
	{
		QDir varRootDirPathQ{ ui->path_work_target->text() };
		varTargetDirPath =
			fs::u8path(varRootDirPathQ.absolutePath().toUtf8().toStdString());
		for (const auto & varI : varPaths) {
			dutys.emplace_back([source = std::get<1>(varI),
				target = varTargetDirPath / std::get<0>(varI)
			]() { try {
				std::error_code e;
				fs::create_directories(target.parent_path(), e);
				fs::copy(source, target, fs::copy_options::overwrite_existing, e);
			}
			catch (...) {}}
			);
		}
	}
	/************************************/
	//give warning here 

	/************************************/
	varPaths.clear();
	std::for_each(std::execution::par_unseq,
		dutys.begin(), dutys.end(),
		[](const auto & f) {f(); });
}
catch (const std::exception & e) {
	ui->errorShow->setPlainText(QString::fromLocal8Bit(e.what()));
}
catch (...) {
	ui->errorShow->setPlainText(QStringLiteral("unknow errror"));
}



