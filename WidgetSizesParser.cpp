#include "WidgetSizesParser.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include "MyCppDifferent.h"
#include "MyQDifferent.h"
#include "MyQString.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"
#include "MyQExecute.h"

WidgetSizesParser::WidgetSizesParser(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	QHBoxLayout *hlo3 = new QHBoxLayout;
	QHBoxLayout *hlo4 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);
	vlo_main->addLayout(hlo3);
	vlo_main->addLayout(hlo4);

	hlo1->addWidget(new QLabel("Path to scan:"));

	//lePathToScan = new QLineEdit("D:\\Documents\\C++ QT\\Notes");
	lePathToScan = new QLineEdit("C:\\");
	hlo1->addWidget(lePathToScan);

	QPushButton *btnScan = new QPushButton("Scan");
	hlo1->addWidget(btnScan);
	connect(btnScan,&QPushButton::clicked, this, &WidgetSizesParser::SlotScan);

	QPushButton *btnPause = new QPushButton("Pause");
	hlo1->addWidget(btnPause);
	connect(btnPause,&QPushButton::clicked, [](){ Item::pause = !Item::pause; });

	QPushButton *btnAbort = new QPushButton("Abort");
	hlo1->addWidget(btnAbort);
	connect(btnAbort,&QPushButton::clicked, [](){ Item::Abort(); });

	QPushButton *btnShowWarnings = new QPushButton(" Show warnings ");
	hlo1->addWidget(btnShowWarnings);
	connect(btnShowWarnings,&QPushButton::clicked, [](){
		MyQDialogs::ShowText("Warnings: has sizes 0 or -1: "+
							 Item::seroSize.join("\n") + "\n" + Item::minusSize.join("\n"));


//		QDir currentDir("C:\\");
//		QFileInfoList entriesFI = currentDir.entryInfoList(
//					QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System,
//					QDir::DirsFirst // сначала папки
//					);

//		int entriesCount = entriesFI.size();

//		QString content;
//		for (int i=0; i<entriesCount; i++)
//			if(entriesFI[i].filePath().startsWith("C:/usersuser1app"))
//				content+= entriesFI[i].filePath()+ " "
//					+ QSn(entriesFI[i].exists()) + " "
//					+ QSn(entriesFI[i].isFile()) + " "
//					+ QSn(entriesFI[i].isDir()) + " "
//					+ QSn(QFile(entriesFI[i].filePath()).open(QFile::ReadOnly)) + " "
//					+ QSn(QFile::copy(entriesFI[i].filePath(), "D:\\for_trash\\"+entriesFI[i].fileName())) + " "
//					+ QSn(QFile::remove(entriesFI[i].filePath())) + " "
//					+ "\n";
//		MyQDialogs::ShowText(content);

	});

	hlo1->addStretch();

	Item::progress2 = new QLabel;
	Item::progress3 = new QLabel;
	hlo2->addWidget(Item::progress2);
	hlo2->addWidget(Item::progress3);
	hlo2->addStretch();

	Item::progress2_2 = new QLabel;
	Item::progress3_2 = new QLabel;
	hlo3->addWidget(Item::progress2_2);
	hlo3->addWidget(Item::progress3_2);
	hlo3->addStretch();

	tree = new QTreeWidget;
	hlo4->addWidget(tree);
	tree->setColumnCount(2);
	tree->setColumnWidth(0, 400);
	tree->setColumnWidth(1, 400);
	tree->setHeaderLabels(QStringList() << "Имя" << "Размер");
	tree->expandToDepth(0); // разворачивает только корень

	QObject::connect(tree, &QTreeWidget::itemExpanded, [this](QTreeWidgetItem *treeItem) {
		if (treeItem->childCount() == 1 &&
			treeItem->child(0)->text(0) == "(загрузка...)") {

			// Очищаем фиктивного ребенка
			treeItem->takeChildren();

			// Получаем указатель
			void* rawPtr = treeItem->data(0, Qt::UserRole).value<void*>();
			auto *item = static_cast<Item*>(rawPtr);
			if (!item) return;

			for (Item &child : item->subitems) {
				CreateTreeItem(treeItem, &child);
			}
		}
	});

	CreateContextMenu();

	resize(900,700);
}

void WidgetSizesParser::CreateContextMenu()
{
	tree->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(tree, &QTreeWidget::customContextMenuRequested,
			this, [this](const QPoint &pos) {
		QTreeWidgetItem* item = tree->itemAt(pos);
		if (!item) return;

		QMenu *menu=new QMenu(this);

		QAction* mOpen = menu->addAction("Open");
		QAction* mShowInExplorer = menu->addAction("Show in explorer");

		connect(mOpen, &QAction::triggered, [item](){
			if(QFileInfo(item->data(1, Qt::UserRole).toString()).isDir())
				MyQExecute::OpenDir(item->data(1, Qt::UserRole).toString());
			else MyQExecute::Execute(item->data(1, Qt::UserRole).toString());
		});
		connect(mShowInExplorer, &QAction::triggered, [item](){
			MyQExecute::ShowInExplorer(item->data(1, Qt::UserRole).toString());
		});

		menu->exec(tree->viewport()->mapToGlobal(pos));
	});
}

WidgetSizesParser::~WidgetSizesParser()
{
}

void WidgetSizesParser::SlotScan()
{
	if(!QFileInfo(lePathToScan->text()).isDir()) { QMbError("not dir"); return; }

	scannedItems = Item::DoCompleteScan(lePathToScan->text());

	tree->clear();
	CreateTreeItem(nullptr, &scannedItems);
}

QTreeWidgetItem *WidgetSizesParser::CreateTreeItem(QTreeWidgetItem *parent, Item *item)
{
	QString sizeStr = item->size == -1
			? ""
			: QLocale().formattedDataSize(item->size);

	QTreeWidgetItem *treeItem = new QTreeWidgetItem(QStringList()
													<< item->m_info.fileName()
													<< sizeStr
													);

	// Связываем item с treeItem через пользовательские данные
	QVariant ptrVariant = QVariant::fromValue<void*>(item);
	treeItem->setData(0, Qt::UserRole, ptrVariant);

	treeItem->setData(1, Qt::UserRole, QVariant(item->m_info.filePath()));

	if (!item->subitems.empty()) {
		// Добавим фиктивного ребенка, чтобы появился значок раскрытия
		treeItem->addChild(new QTreeWidgetItem(QStringList() << "(загрузка...)"));
	}

	if (parent)
		parent->addChild(treeItem);
	else
		tree->addTopLevelItem(treeItem);

	return treeItem;
}

void Item::InitAndScan(QFileInfo &&info, int deps_)
{
	while(pause and !abort) { MyCppDifferent::sleep_ms(10); QCoreApplication::processEvents(); }
	if(abort) { return; }

	depth = deps_+1;
	m_info = std::move(info);
	size = m_info.isDir() ? -1 : m_info.size();

	if(m_info.isSymLink()) { return; }

	if (!m_info.isDir()) // если это не каталог
	{
		if(m_info.size() == 0) seroSize += "size = 0 " + m_info.filePath();
		else if(m_info.size() == -1) minusSize += "size = -1 " + m_info.filePath();
		return;
	}

	QDir currentDir(m_info.filePath());
	QFileInfoList entriesFI = currentDir.entryInfoList(
				QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System,
				QDir::DirsFirst // сначала папки
				);

	int entriesCount = entriesFI.size();

	subitems.clear();
	subitems.reserve(entriesCount);

	size = 0;
	for (int i=0; i<entriesCount; i++) {

		PrintProgress(i, entriesCount, entriesFI[i]);

		while(pause and !abort) { MyCppDifferent::sleep_ms(10); QCoreApplication::processEvents(); }
		if(abort) { return; }

		subitems.emplace_back(std::move(entriesFI[i]), depth);
		size += subitems.back().size;

		PrintProgress(i+1, entriesCount, entriesFI[i]);
	}
}

void Item::PrintProgress(int n, int total, const QFileInfo &childFI)
{
	if(depth == 1) {
		progress2->setText("Обработано " + QSn(n) + " из " + QSn(total));
		progress2_2->setText(m_info.filePath());
	}
	if(depth == 2){
		progress3->setText(":  " + QSn(n) + " из " + QSn(total));
		progress3_2->setText("  "+m_info.fileName() + "    /    " + childFI.fileName());
	}

	QCoreApplication::processEvents();
}

Item Item::DoCompleteScan(QString path)
{
	abort = false;
	pause = false;
	seroSize.clear();
	minusSize.clear();

	Item result(QFileInfo(path), 0);

	if(seroSize.empty() and minusSize.empty()) QMbInfo("Complete");
	else QMbInfo("Complete with warnings");

	return result;
}
