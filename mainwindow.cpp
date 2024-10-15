#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <QImage>

#include <iostream>
#include <memory>
#include <thread>
#include <iomanip>
#include <map>
#include "image.h"

void MainWindow::requestComplete(Request *request)
{
    if (request->status() == Request::RequestCancelled)
       return;

    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();

    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence
              << " timestamp: " << metadata.timestamp
              << " bytesused: ";

        unsigned int nplane = 0;
        for (const FrameMetadata::Plane &plane : metadata.planes())
        {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size())
            {
                std::cout << "/";
            }
        }

        std::cout << std::endl;

//        std::unique_ptr<Image> image = Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
//        libcamera::Span<const uint8_t> data = image->data(0);
//        QImage qImage(data.data(), 640, 480, QImage::Format_RGB888);
//        ui->label->setPixmap(QPixmap::fromImage(qImage));
    }


    request->reuse(libcamera::Request::ReuseBuffers);
    camera->queueRequest(request);

}

std::string cameraName(Camera *camera)
{
    const ControlList &props = camera->properties();
    std::string name;

    const auto &location = props.get(properties::Location);
    if (location) {
        switch (*location) {
        case properties::CameraLocationFront:
            name = "Internal front camera";
            break;
        case properties::CameraLocationBack:
            name = "Internal back camera";
            break;
        case properties::CameraLocationExternal:
            name = "External camera";
            const auto &model = props.get(properties::Model);
            if (model)
                name = " '" + *model + "'";
            break;
        }
    }

    name += " (" + camera->id() + ")";

    return name;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cm = std::make_unique<CameraManager>();
    cm->start();

    for (auto const &camera : cm->cameras())
    {
        std::cout << " - " << cameraName(camera.get()) << std::endl;
    }

    auto cameras = cm->cameras();
    if (cameras.empty())
    {
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return;
    }

    std::string cameraId = cameras[0]->id();

    camera = cm->get(cameraId);
    camera->acquire();

    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );

    StreamConfiguration &streamConfig = config->at(0);
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;

    config->validate();
    std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;

    camera->configure(config.get());

    allocator = new FrameBufferAllocator(camera);

    for (StreamConfiguration &cfg : *config) {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request" << std::endl;
            return;
        }

        requests.push_back(std::move(request));
    }

    camera->requestCompleted.connect(this, &MainWindow::requestComplete);

    camera->start();

    for (std::unique_ptr<Request> &request : requests)
    {
       camera->queueRequest(request.get());
    }
}

MainWindow::~MainWindow()
{
    camera->stop();
    allocator->free(stream);
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();
    delete ui;
}
