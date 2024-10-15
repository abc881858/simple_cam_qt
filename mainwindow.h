#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QImage>
#include <libcamera/libcamera.h>
#include <atomic>
#include <map>
#include <QQueue>

using namespace libcamera;
using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void requestComplete(Request *request);
private:
    Ui::MainWindow *ui;
    std::unique_ptr<CameraManager> cm;
    FrameBufferAllocator *allocator;
    std::vector<std::unique_ptr<Request>> requests;
    std::shared_ptr<Camera> camera;
    Stream *stream;
};
