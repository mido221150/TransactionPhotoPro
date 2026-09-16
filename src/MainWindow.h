#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QImage>
#include <QPrinter>
#include <QSizeF>
#include <vector>

struct PhotoItem {
    QImage original;
    QImage processed;
    QString name;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private slots:
    void addPhotos();
    void addFolder();
    void removeSelected();
    void clearPhotos();
    void processAll();
    void resetProcessing();
    void updatePreview();
    void savePdf();
    void printPage();
    void saveImages();
    void moveSelectedUp();
    void moveSelectedDown();
    void autoNumberNames();
private:
    void buildUi();
    void addImageFile(const QString &f);
    void rebuildPage(QPainter *external=nullptr, const QRectF &target=QRectF());
    struct LayoutInfo {
        int columns{0};
        int rows{0};
        bool rotated{false};
        QSizeF photoCm;
    };
    LayoutInfo calculateLayout() const;
    QSizeF selectedPhotoSize() const;
    QImage processImage(const QImage &in) const;
    QImage sharpen(const QImage &in, int amount) const;
    QImage removeLightBackground(const QImage &in) const;
    QSize paperPixels() const;
    void loadSettings();
    void saveSettings();
    std::vector<PhotoItem> photos;
    QListWidget *list{};
    QGraphicsView *view{};
    QGraphicsScene *scene{};
    QComboBox *paper{}, *photoSize{}, *quality{};
    QSpinBox *columns{}, *rows{}, *fontSize{};
    QSpinBox *bgTolerance{}, *bgFeather{};
    QCheckBox *autoLayout{}, *removeBg{}, *whiteBg{}, *showNames{};
    QLineEdit *nameEdit{};
    QDoubleSpinBox *margin{}, *gap{};
    QProgressBar *progress{};
    QLabel *status{}, *layoutStatus{};
};
