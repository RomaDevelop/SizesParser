#ifndef WIDGETSIZESPARSER_H
#define WIDGETSIZESPARSER_H

#include <QWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QLineEdit>
#include <QFileInfo>
#include <QLabel>

struct Item
{
	QFileInfo m_info;
	qint64 size = -1;
	int depth = 0;
	std::vector<Item> subitems;

	Item() {}
	Item(QFileInfo &&info, int deps_) { InitAndScan(std::move(info), deps_); }
	void InitAndScan(QFileInfo &&info, int deps_);

	void PrintProgress(int n, int total, const QFileInfo &childFI);

	inline static QLabel *progress2;
	inline static QLabel *progress3;
	inline static QLabel *progress2_2;
	inline static QLabel *progress3_2;

	inline static Item DoCompleteScan(QString path);
	inline static void Abort() { abort=true; }
	inline static bool pause = false;

	inline static bool abort = false;
	inline static QStringList seroSize;
	inline static QStringList minusSize;
};

class WidgetSizesParser : public QWidget
{
	Q_OBJECT

public:
	WidgetSizesParser(QWidget *parent = nullptr);
	void CreateContextMenu();
	~WidgetSizesParser();

	QTreeWidget *tree;
	QLineEdit *lePathToScan;

	void SlotScan();

	Item scannedItems;

	QTreeWidgetItem* CreateTreeItem(QTreeWidgetItem *parent, Item *item);
};

#endif // WIDGETSIZESPARSER_H
