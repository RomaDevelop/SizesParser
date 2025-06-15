#include "WidgetSizesParser.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	WidgetSizesParser w;
	w.show();
	return a.exec();
}
