#include <QApplication>
#include <QFont>

#include <jsonmodel.h>
#include <jsonview.h>


int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

    JsonModel model; // (argv[1]);

    JsonView win;
    win.setModel(&model);
    win.show();

	return app.exec();
}
