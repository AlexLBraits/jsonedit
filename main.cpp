#include <QApplication>
#include <jsonmodel.h>
#include <jsonview.h>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

    JsonModel model;

    JsonView win;
    win.setModel(&model);
    win.show();


	return app.exec();
}
