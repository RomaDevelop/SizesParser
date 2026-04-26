#include "WidgetSizesParser.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressDialog>
#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>

#include <functional>

#include "MyCppDifferent.h"
#include "MyQDifferent.h"
#include "MyQString.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"
#include "MyQExecute.h"
#include "WidgetSizesParser.h"

#include "Code.h"
#include "WidgetSizesParser.h"

QString settingsFile;
int maxWidth;

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

	lePathToScan = new QLineEdit("C:\\");
	hlo1->addWidget(lePathToScan);

	QPushButton *btnScan = new QPushButton("Scan");
	hlo1->addWidget(btnScan);
	connect(btnScan, &QPushButton::clicked, this, &WidgetSizesParser::SlotScan);

	Item::btnPause  = new QPushButton("Pause");
	hlo1->addWidget(Item::btnPause);
	connect(Item::btnPause, &QPushButton::clicked, [](){
		Item::pause = !Item::pause;
	});

	QPushButton *btnAbort = new QPushButton("Stop");
	hlo1->addWidget(btnAbort);
	connect(btnAbort,&QPushButton::clicked, [](){ Item::Abort(); });

	QPushButton *btnShowWarnings = new QPushButton(" Show warnings ");
	hlo1->addWidget(btnShowWarnings);
	connect(btnShowWarnings, &QPushButton::clicked, [](){
		MyQDialogs::ShowText("Warnings: has sizes 0 or -1: "
							 + Item::seroSize.join("\n")
							 + "\n\n" + Item::minusSize.join("\n")
							 + "\n\n" + Item::symLinkWorked.join("\n"));
	});

	hlo1->addStretch();

	QPushButton *btnOpen = new QPushButton();
	hlo1->addWidget(btnOpen);
	btnOpen->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_DialogOpenButton));
	connect(btnOpen,&QPushButton::clicked, [this](){ LoadItems(); });

	QPushButton *btnCheck = new QPushButton();
	hlo1->addWidget(btnCheck);
	btnCheck->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_BrowserReload));
	connect(btnCheck,&QPushButton::clicked, [this](){ CheckItems(); });

	QPushButton *btnSave = new QPushButton();
	hlo1->addWidget(btnSave);
	btnSave->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_DialogSaveButton));
	connect(btnSave,&QPushButton::clicked, [this](){ SaveItems(scannedItems); });

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

			for (auto &child : item->subitems) {
				CreateTreeItem(treeItem, child.get());
			}
		}
	});

	CreateContextMenu();

	resize(900,700);
	maxWidth = width();

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QSettings settings(settingsFile, QSettings::IniFormat);
	dirSavedScans = settings.value("dirSavedScans").toString();
}

void WidgetSizesParser::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	maxWidth = width();
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
		QAction* mRecheck = menu->addAction("Reparse");

		connect(mOpen, &QAction::triggered, [item](){
			if(QFileInfo(item->data(1, Qt::UserRole).toString()).isDir())
				MyQExecute::OpenDir(item->data(1, Qt::UserRole).toString());
			else MyQExecute::Execute(item->data(1, Qt::UserRole).toString());
		});
		connect(mShowInExplorer, &QAction::triggered, [item](){
			MyQExecute::ShowInExplorer(item->data(1, Qt::UserRole).toString());
		});
		connect(mRecheck, &QAction::triggered, this, [this, item](){
			RecheckTreeItem(item);
		});

		menu->exec(tree->viewport()->mapToGlobal(pos));
	});
}

WidgetSizesParser::~WidgetSizesParser()
{
	QSettings settings(settingsFile, QSettings::IniFormat);
	settings.setValue("dirSavedScans", dirSavedScans);
}

void WidgetSizesParser::SlotScan()
{
	QString path = lePathToScan->text();
	if(!QFileInfo(path).isDir()) { QMbError("not dir"); return; }

	if(QFileInfo(path).isSymLink()) { QMbError("Target is sym link"); return; }
	scannedItems = Item::DoCompleteScan(path);

	tree->clear();
	CreateTreeItem(nullptr, &scannedItems);
}

void WidgetSizesParser::SaveItems(const Item &items)
{
	auto file = QFileDialog::getSaveFileName(this, "Select file", dirSavedScans, "*.txt");
	if(file.isEmpty()) return;
	dirSavedScans = QFileInfo(file).path();

	QFile qfile(file);
	if(!qfile.open(QFile::WriteOnly))
	{
		QMbError("Can't open file "+file);
		return;
	}

	QTextStream stream(&qfile);
	stream.setCodec(QTextCodec::codecForName("UTF-8"));

	QProgressDialog dialog(this);
	dialog.setMinimumDuration(1000);
	dialog.setWindowTitle("Подготовка");
	dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	dialog.setCancelButton(nullptr);
	dialog.setRange(0, 0);

	QApplication::processEvents();
	auto all = items.ItemsList();

	dialog.setRange(0, all.size());
	dialog.setWindowTitle("Запись");
	dialog.setValue(0);
	QApplication::processEvents();
	for(auto &item:all)
	{
		item->SaveItem(stream, false);
		dialog.setValue(dialog.value()+1);
		QApplication::processEvents();
	}
}

void WidgetSizesParser::LoadItems()
{
	auto fileName = QFileDialog::getOpenFileName(this, "Select file", dirSavedScans, "*.txt");
	if (fileName.isEmpty()) return;
	dirSavedScans = QFileInfo(fileName).path();

	QFile qfile(fileName);
	if (!qfile.open(QFile::ReadOnly)) {
		QMbError("Can't open file " + fileName);
		return;
	}

	qint64 totalSize = qfile.size();
	int readedLinesCnt = 0;

	QTextStream stream(&qfile);
	stream.setCodec(QTextCodec::codecForName("UTF-8"));

	QVector<Item*> stack;
	scannedItems = Item();

	QProgressDialog dialog(this);
	dialog.setMinimumDuration(1000);
	dialog.setWindowTitle("Чтение");
	dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	dialog.setCancelButton(nullptr);
	dialog.setRange(0, 100);

	while (!stream.atEnd()) {
		QString line = stream.readLine();
		readedLinesCnt++;

		if(readedLinesCnt % 300 == 0)
		{
			int percent = static_cast<int>((qfile.pos() * 100) / totalSize);
			dialog.setValue(percent);
			QApplication::processEvents();
		}

		if (line.trimmed().isEmpty()) continue;

		Item newItem;
		if (QString error = newItem.LoadItem(line); !error.isEmpty()) {
			QMbError("Line parse error: " + error);
			return;
		}

		// Если это первый элемент файла (корень)
		if (stack.isEmpty()) {
			if(newItem.depth != 1)
			{
				QMbError("Error depth in line: " + line);
				return;
			}
			scannedItems = std::move(newItem);
			stack.push_back(&scannedItems);
			continue;
		}

		// Ищем родителя: поднимаемся по стеку, пока глубина последнего в стеке не станет меньше текущего
		while (!stack.isEmpty() && stack.back()->depth >= newItem.depth) {
			stack.pop_back();
		}

		if (stack.isEmpty()) {
			QMbError("Ошибка структуры: обнаружен лишний корневой элемент или нарушена вложенность");
			return;
		}

		// 4. Добавление элемента
		stack.back()->subitems.emplace_back(std::make_unique<Item>(std::move(newItem)));
		stack.back()->subitems.back()->parent = stack.back();
		// Кладем указатель на только что добавленный элемент в стек
		stack.push_back(stack.back()->subitems.back().get());
	}

	tree->clear();
	CreateTreeItem(nullptr, &scannedItems);
	QMbInfo("Чтение завершено");
}

void WidgetSizesParser::CheckItems()
{
	auto alternalScannedItems = Item::DoCompleteScan(scannedItems.itemPathWithName);

	if(alternalScannedItems == scannedItems)
	{
		QMbInfo("Сверка отличий не обнаружила");
	}
	else
	{
		auto answ = QMessageBox::question(this, "", "При сверке обнаружены отличия, сохранить их?");
		if(answ == QMessageBox::Yes)
		{
			SaveItems(alternalScannedItems);
		}
	}
}

void WidgetSizesParser::RecheckTreeItem(QTreeWidgetItem *treeItem)
{
	if(!treeItem)
		return;

	auto *item = static_cast<Item*>(treeItem->data(0, Qt::UserRole).value<void*>());
	if(!item)
	{
		QMbError("Не удалось получить элемент для перепроверки");
		return;
	}

	const QString path = item->itemPathWithName;
	const QFileInfo info(path);

	if(!info.exists())
	{
		auto answer = QMessageBox::question(this, "", "Перепроверяемый элемент не существует. Удалить его из списка?");
		if(answer != QMessageBox::Yes)
			return;

		if(treeItem == tree->topLevelItem(0))
		{
			scannedItems = Item();
			tree->clear();
			return;
		}

		if(!RemoveItem(item))
		{
			QMbError("Не удалось удалить элемент из списка");
			return;
		}

		delete treeItem;
		return;
	}

	Item::progressRootDepth = item->depth;
	Item updatedItem(QFileInfo(path), item->depth - 1);
	const QString diff = Compare(*item, updatedItem);
	if(diff.isEmpty())
	{
		QMbInfo("Отличий не обнаружено");
		return;
	}

	auto answer = QMessageBox::question(this, "", "Обнаружено несовпадение. Обновить элемент?");
	if(answer != QMessageBox::Yes)
		return;

	item->InitFromItem(std::move(updatedItem), false);
	UpdateTreeItemView(treeItem, item);
}

QTreeWidgetItem *WidgetSizesParser::CreateTreeItem(QTreeWidgetItem *parent, Item *item)
{
	QString sizeStr = item->size == -1
			? ""
			: QLocale().formattedDataSize(item->size);

	QStringList vals = { item->itemNameNoPath, sizeStr };

	QTreeWidgetItem *treeItem = new QTreeWidgetItem(vals);

	// Связываем item с treeItem через пользовательские данные
	QVariant ptrVariant = QVariant::fromValue<void*>(item);
	treeItem->setData(0, Qt::UserRole, ptrVariant);

	treeItem->setData(1, Qt::UserRole, QVariant(item->itemPathWithName));

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

void WidgetSizesParser::UpdateTreeItemView(QTreeWidgetItem *treeItem, Item *item)
{
	if(!treeItem || !item)
		return;

	QString sizeStr = item->size == -1
			? ""
			: QLocale().formattedDataSize(item->size);

	treeItem->setText(0, item->itemNameNoPath);
	treeItem->setText(1, sizeStr);
	treeItem->setData(0, Qt::UserRole, QVariant::fromValue<void*>(item));
	treeItem->setData(1, Qt::UserRole, QVariant(item->itemPathWithName));

	treeItem->takeChildren();
	if(!item->subitems.empty())
		treeItem->addChild(new QTreeWidgetItem(QStringList() << "(загрузка...)"));
}

bool WidgetSizesParser::RemoveItem(Item *itemToRemove)
{
	if(!itemToRemove)
		return false;

	std::function<bool(Item&)> removeFrom = [&](Item &parentItem)
	{
		for(auto it = parentItem.subitems.begin(); it != parentItem.subitems.end(); ++it)
		{
			if(it->get() == itemToRemove)
			{
				parentItem.subitems.erase(it);
				return true;
			}

			if(removeFrom(*it->get()))
				return true;
		}

		return false;
	};

	return removeFrom(scannedItems);
}

void Item::InitFromItem(Item && srcItem, bool setParentFromSrc)
{
	itemPathWithName = std::move(srcItem.itemPathWithName);
	itemNameNoPath = std::move(srcItem.itemNameNoPath);
	size = srcItem.size;
	depth = srcItem.depth;
	subitems = std::move(srcItem.subitems);
	for(auto &subitem : subitems)
	{
		subitem->parent = this;
	}
	if(setParentFromSrc) parent = srcItem.parent;
}

void Item::InitAndScan(QFileInfo &&info, int deps_)
{
#define PAUSE { Item::btnPause->setText("Continue"); MyCppDifferent::sleep_ms(10); QCoreApplication::processEvents(); }

	while(pause and !abort) PAUSE
			Item::btnPause->setText("Pause");
	if(abort) { return; }

	depth = deps_+1;
	size = info.isDir() ? -1 : info.size();

	itemPathWithName = info.filePath();
	itemNameNoPath = info.fileName();
	if(itemNameNoPath.isEmpty() and info.isRoot()) itemNameNoPath = itemPathWithName;

	if(info.isSymLink()) { symLinkWorked += "sym link " + itemPathWithName; return; }

	if (!info.isDir()) // если это не каталог
	{
		if(info.size() == 0) seroSize += "size = 0 " + info.filePath();
		else if(info.size() == -1) minusSize += "size = -1 " + info.filePath();
		return;
	}

	QDir currentDir(info.filePath());
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

		while(pause and !abort) PAUSE
		Item::btnPause->setText("Pause");
		if(abort) { return; }

		if(not entriesFI[i].isSymLink())
		{
			subitems.emplace_back(std::make_unique<Item>(std::move(entriesFI[i]), depth));
			subitems.back()->parent = this;
			size += subitems.back()->size;
		}

		PrintProgress(i+1, entriesCount, entriesFI[i]);
	}
}

std::vector<Item*> Item::ItemsList() const
{
	std::vector<Item*> vect;
	ItemsList(vect);
	return vect;
}

void Item::ItemsList(std::vector<Item*> &vect) const
{
	vect.push_back(const_cast<Item*>(this));
	for(auto &item:subitems) item->ItemsList(vect);
}

void Item::SaveItem(QTextStream &stream, bool deep) const
{
	stream << "\"" << itemPathWithName << "\" \"" << itemNameNoPath << "\" " << size << " " << depth << "\n";
	if(deep) for(auto &item:subitems) item->SaveItem(stream, deep);
}

QString Item::LoadItem(const QString &line)
{
	auto words = Code::CommandToWords(line);
	if(words.size() < 4) return "words size error ("+QSn(words.size())+")";
	if(not TextConstant::IsItTextConstant(words[0], false)) return "word 0 ("+words[0]+") is not text constant";
	if(not TextConstant::IsItTextConstant(words[1], false)) return "word 1 ("+words[1]+") is not text constant";

	itemPathWithName = TextConstant::GetTextConstVal(words[0], false);
	itemNameNoPath = TextConstant::GetTextConstVal(words[1], false);

	bool ok;
	size = words[2].toLongLong(&ok);
	if(not ok) return "word 2 ("+words[2]+") is not ulonglong";

	depth = words[3].toUShort(&ok);
	if(not ok) return "word 3 ("+words[3]+") is not ushort";

	return "";
}

void Item::PrintProgress(int n, int total, const QFileInfo &childFI)
{
	static bool scanningWholeDrive = false;
	if(depth == progressRootDepth) {
		progress2->setText("Обработано " + QSn(n) + " из " + QSn(total));
		progress2_2->setText(progress2_2->fontMetrics().elidedText(itemPathWithName, Qt::ElideRight, maxWidth/3));
		if(itemPathWithName.endsWith("/")) scanningWholeDrive = true;
		else scanningWholeDrive = false;
	}
	if(depth == progressRootDepth + 1){
		progress3->setText(":  " + QSn(n) + " из " + QSn(total));
		if(scanningWholeDrive) progress3_2->setText(progress3_2->fontMetrics().elidedText(
					"    "+itemNameNoPath + "    /    " + childFI.fileName(), Qt::ElideRight, maxWidth/3));
		else progress3_2->setText(progress3_2->fontMetrics().elidedText(
					"    /    "+itemNameNoPath + "    /    " + childFI.fileName(), Qt::ElideRight, maxWidth/3));
	}

	QCoreApplication::processEvents();
}

Item Item::DoCompleteScan(QString path)
{
	abort = false;
	pause = false;
	Item::btnPause->setText("Pause");
	progressRootDepth = 1;
	seroSize.clear();
	minusSize.clear();
	symLinkWorked.clear();

	Item result(QFileInfo(path), 0);

	if(seroSize.empty() and minusSize.empty()) QMbInfo("Complete");
	else QMbInfo("Complete with warnings");

	return result;
}

QString Compare(const Item &l, const Item &r)
{
	if(r.itemPathWithName != l.itemPathWithName) return r.itemPathWithName + " != " + l.itemPathWithName;
	if(r.itemNameNoPath != l.itemNameNoPath) return r.itemNameNoPath + " != " + l.itemNameNoPath;
	if(r.size != l.size) return QSn(r.size) + " != " + QSn(l.size);
	if(r.depth != l.depth) return QSn(r.depth) + " != " + QSn(l.depth);
	if(r.subitems.size() != l.subitems.size()) return QSn(r.subitems.size()) + " != " + QSn(l.subitems.size());
	for(uint i=0; i<l.subitems.size(); i++)
	{
		auto res = Compare(*l.subitems[i], *r.subitems[i]);
		if(not res.isEmpty()) return res;
	}
	return "";
}

bool operator==(const Item &l, const Item &r)
{
	return Compare(l, r).isEmpty();
}

bool operator!=(const Item &l, const Item &r)
{
	return not (l == r);
}
