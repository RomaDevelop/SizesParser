#ifndef WIDGETSIZESPARSER_H
#define WIDGETSIZESPARSER_H

#include <memory>

#include <QTextStream>
#include <QWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QLineEdit>
#include <QFileInfo>
#include <QLabel>

struct Item;

using ItemUptr = std::unique_ptr<Item>;

struct Item
{
	QString itemPathWithName;
	QString itemNameNoPath;
	qint64 size = -1;
	int depth = 0;
	std::vector<ItemUptr> subitems;
	Item *parent = nullptr;

	Item() {}
	Item(QFileInfo &&info, int deps_) { InitAndScan(std::move(info), deps_); }
	void InitFromItem(Item &&srcItem, bool setParentFromSrc);
	void InitAndScan(QFileInfo &&info, int deps_);
	std::vector<Item*> ItemsList() const;
	void ItemsList(std::vector<Item*> &vect) const;

	void SaveItem(QTextStream &stream, bool deep) const;
	QString LoadItem(const QString &line);

	void PrintProgress(int n, int total, const QFileInfo &childFI);

	inline static int progressRootDepth = 1;
	inline static int progressMaxWidth;
	inline static QLabel *progress2;
	inline static QLabel *progress3;
	inline static QLabel *progress2_2;
	inline static QLabel *progress3_2;

	inline static QPushButton *btnPause;

	inline static Item DoCompleteScan(QString path);
	inline static void Abort() { abort=true; }
	inline static bool pause = false;

	inline static bool abort = false;
	inline static QStringList seroSize;
	inline static QStringList minusSize;
	inline static QStringList symLinkWorked;
};

inline QString Compare(const Item &l, const Item &r);
inline bool operator== (const Item &l, const Item &r);
inline bool operator!= (const Item &l, const Item &r);

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
	void SaveItems(const Item &items);
	void LoadItems();
	void CheckItems();

	QTreeWidgetItem* CreateTreeItem(QTreeWidgetItem *parent, Item *item);
	void RecheckTreeItem(QTreeWidgetItem *treeItem);
	void UpdateTreeItemView(QTreeWidgetItem *treeItem, Item *item);
	bool RemoveItem(Item *itemToRemove);

	QString dirSavedScans;

protected:
	void resizeEvent(QResizeEvent *event) override;
};

#endif // WIDGETSIZESPARSER_H
