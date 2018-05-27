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
	class Pack {
	public:
		fs::path source;
		fs::path target;
		std::string error_string;
	};
	std::vector< std::shared_ptr<Pack> > dutys_pack;
	std::vector< std::function<void(void)> > dutys;
	dutys.reserve(varPaths.size());
	{
		QDir varRootDirPathQ{ ui->path_work_target->text() };
		varTargetDirPath =
			fs::u8path(varRootDirPathQ.absolutePath().toUtf8().toStdString());
		for (const auto & varI : varPaths) {
			std::shared_ptr<Pack> varPackD = dutys_pack.emplace_back(new Pack);
			varPackD->source = std::get<1>(varI);
			varPackD->target = varTargetDirPath / std::get<0>(varI);
			dutys.emplace_back([varPackD]() { try {
				const auto & target = varPackD->target;
				const auto & source = varPackD->source;
				std::error_code e;
				fs::create_directories(target.parent_path(), e)/*ignore the error*/;
				fs::copy(source, target, fs::copy_options::overwrite_existing);
			}
			catch (const std::exception &e) {
				varPackD->error_string = e.what();
			}
			catch (...) { varPackD->error_string = "unknow error"; }}
			);
		}
	}
	/************************************/
	//give warning here 
	{
		int count = 0;
		QString varAboutToCopy;
		for ( const auto & varI : dutys_pack ) {
			varAboutToCopy += QString::fromUtf16(varI->source.u16string().c_str());
			varAboutToCopy += QStringLiteral( R"(
F&T:
)" );
			varAboutToCopy += QString::fromUtf16(varI->target.u16string().c_str());
			varAboutToCopy += QStringLiteral(R"(
)");
			if ((++count) > 6/*最大预览数量*/) { break; }
		}
		if(QMessageBox::Ok!= QMessageBox::warning(nullptr, 
			QStringLiteral("确认执行?"), 
			varAboutToCopy,
			QMessageBox::Ok,
			QMessageBox::Ignore))
			return;
	}
	/************************************/
	varPaths.clear();
	std::for_each(std::execution::par_unseq,
		dutys.begin(), dutys.end(),
		[](const auto & f) {f(); });
	/*show log**************************/
	{

		QString varAboutToCopy;
		for (const auto & varI : dutys_pack) {
			varAboutToCopy += QStringLiteral(R"(复制日志:-----------------------
从:)");
			varAboutToCopy += QString::fromUtf16(varI->source.u16string().c_str());
			varAboutToCopy += QStringLiteral(R"(
到:)");
			varAboutToCopy += QString::fromUtf16(varI->target.u16string().c_str());
			varAboutToCopy += QStringLiteral(R"(
错误内容:)");
			varAboutToCopy += QString::fromLocal8Bit( varI->error_string.size()?
				varI->error_string.c_str():"Ok");
			varAboutToCopy += QStringLiteral(R"(
)");
		}
		varAboutToCopy += QStringLiteral(R"(Raw文件列表:
)");
		for (const auto & varI : dutys_pack) {
			varAboutToCopy += QString::fromUtf16( varI->target.filename().u16string().c_str() );
			varAboutToCopy += QStringLiteral(R"(
)");
		}
		ui->errorShow->setPlainText(varAboutToCopy);
	}
}
catch (const std::exception & e) {
	ui->errorShow->setPlainText(QString::fromLocal8Bit(e.what()));
}
catch (...) {
	ui->errorShow->setPlainText(QStringLiteral("unknow errror"));
}



