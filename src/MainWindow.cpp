#include "MainWindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QImageReader>
#include <QPageSize>
#include <QPageLayout>
#include <QtMath>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QSettings>
#include <QUrl>
#include <QQueue>
#include <algorithm>
#include <limits>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("معاملات Photo Pro | تجهيز صور المعاملات للطباعة");
    setWindowIcon(QIcon(":/icon.png"));
    resize(1500, 920);
    setAcceptDrops(true);
    buildUi();
    loadSettings();
    updatePreview();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if(view && scene && !scene->sceneRect().isEmpty())
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    int added=0;
    for(const QUrl &u : urls){
        QString f = u.toLocalFile();
        if(f.isEmpty()) continue;
        QFileInfo fi(f);
        if(fi.isDir()){
            QDirIterator it(f,{"*.jpg","*.jpeg","*.png","*.bmp","*.webp"},QDir::Files,QDirIterator::Subdirectories);
            while(it.hasNext()){ addImageFile(it.next()); ++added; }
        } else {
            addImageFile(f); ++added;
        }
    }
    if(added>0){
        status->setText(QString("تمت إضافة %1 صورة بالسحب والإفلات").arg(added));
        updatePreview();
    }
    event->acceptProposedAction();
}

void MainWindow::loadSettings()
{
    QSettings s("TransactionPhotoPro","TransactionPhotoPro");
    paper->setCurrentIndex(qBound(0,s.value("paper",0).toInt(),3));
    quality->setCurrentIndex(s.value("quality",1).toInt());
    removeBg->setChecked(s.value("removeBg",true).toBool());
    whiteBg->setChecked(s.value("whiteBg",true).toBool());
    showNames->setChecked(s.value("showNames",true).toBool());
    fontSize->setValue(s.value("fontSize",17).toInt());
    margin->setValue(s.value("margin",8.0).toDouble());
    gap->setValue(s.value("gap",4.0).toDouble());
    bgTolerance->setValue(s.value("bgTolerance",23).toInt());
    bgFeather->setValue(s.value("bgFeather",2).toInt());
}

void MainWindow::saveSettings()
{
    QSettings s("TransactionPhotoPro","TransactionPhotoPro");
    s.setValue("paper",paper->currentIndex());
    s.setValue("quality",quality->currentIndex());
    s.setValue("removeBg",removeBg->isChecked());
    s.setValue("whiteBg",whiteBg->isChecked());
    s.setValue("showNames",showNames->isChecked());
    s.setValue("fontSize",fontSize->value());
    s.setValue("margin",margin->value());
    s.setValue("gap",gap->value());
    s.setValue("bgTolerance",bgTolerance->value());
    s.setValue("bgFeather",bgFeather->value());
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *main = new QHBoxLayout(central);
    main->setContentsMargins(14,14,14,14);
    main->setSpacing(14);

    auto *side = new QFrame;
    side->setObjectName("side");
    side->setMinimumWidth(390);
    auto *s = new QVBoxLayout(side);
    s->setSpacing(10);

    auto *title = new QLabel("معاملات Photo Pro");
    title->setObjectName("title");
    auto *sub = new QLabel("تجهيز • تحسين • إزالة خلفية • طباعة");
    sub->setObjectName("sub");
    s->addWidget(title);
    s->addWidget(sub);

    auto *add = new QPushButton("إضافة صور");
    add->setObjectName("primary");
    auto *folder = new QPushButton("إضافة مجلد");
    folder->setObjectName("secondary");
    auto *del = new QPushButton("حذف المحدد");
    del->setObjectName("danger");
    auto *clear = new QPushButton("تفريغ الكل");
    clear->setObjectName("danger");
    connect(add,&QPushButton::clicked,this,&MainWindow::addPhotos);
    connect(folder,&QPushButton::clicked,this,&MainWindow::addFolder);
    connect(del,&QPushButton::clicked,this,&MainWindow::removeSelected);
    connect(clear,&QPushButton::clicked,this,&MainWindow::clearPhotos);
    auto *photoActions = new QGroupBox("إدارة الصور");
    auto *photoActionLayout = new QGridLayout(photoActions);
    photoActionLayout->setHorizontalSpacing(8);
    photoActionLayout->setVerticalSpacing(8);
    photoActionLayout->addWidget(add, 0, 0);
    photoActionLayout->addWidget(folder, 0, 1);
    photoActionLayout->addWidget(del, 1, 0);
    photoActionLayout->addWidget(clear, 1, 1);
    s->addWidget(photoActions);

    list = new QListWidget;
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setIconSize(QSize(48,48));
    list->setAlternatingRowColors(true);
    list->setMovement(QListView::Static);
    list->setSpacing(4);
    auto *listTitle = new QLabel("الصور (يمكن اختيار أكثر من صورة، ويدعم السحب من خارج البرنامج)");
    listTitle->setObjectName("listTitle");
    s->addWidget(listTitle);
    s->addWidget(list,1);

    auto *moveRow = new QHBoxLayout;
    auto *up = new QPushButton("تحريك لأعلى");
    up->setObjectName("secondary");
    auto *down = new QPushButton("تحريك لأسفل");
    down->setObjectName("secondary");
    auto *autoNum = new QPushButton("ترقيم تلقائي للأسماء");
    autoNum->setObjectName("secondary");
    connect(up,&QPushButton::clicked,this,&MainWindow::moveSelectedUp);
    connect(down,&QPushButton::clicked,this,&MainWindow::moveSelectedDown);
    connect(autoNum,&QPushButton::clicked,this,&MainWindow::autoNumberNames);
    moveRow->addWidget(up); moveRow->addWidget(down);
    auto *orderingGroup = new QGroupBox("ترتيب الصور");
    auto *orderingLayout = new QVBoxLayout(orderingGroup);
    orderingLayout->addLayout(moveRow);
    orderingLayout->addWidget(autoNum);
    s->addWidget(orderingGroup);

    auto *paperGroup = new QGroupBox("حجم الورق والتخطيط التلقائي");
    auto *form = new QFormLayout(paperGroup);
    paper = new QComboBox;
    paper->addItems({"A4","A5","10 × 15 سم","Letter"});
    photoSizeLabel = new QLabel("4 × 6 سم");
    photoSizeLabel->setObjectName("fixedPhotoSize");
    photoSizeLabel->setAlignment(Qt::AlignCenter);
    photoSizeLabel->setToolTip("مقاس الصور ثابت في هذا الإصدار");
    quality = new QComboBox;
    quality->addItems({"تحسين خفيف","تحسين متوسط","تحسين قوي"});
    removeBg = new QCheckBox("إزالة الخلفية البيضاء/الفاتحة");
    removeBg->setChecked(true);
    bgTolerance = new QSpinBox; bgTolerance->setRange(5,120); bgTolerance->setValue(23);
    bgTolerance->setToolTip("كلما زادت القيمة، زادت درجة الفتح التي تُعتبر خلفية وتُزال");
    bgFeather = new QSpinBox; bgFeather->setRange(0,8); bgFeather->setValue(2);
    bgFeather->setToolTip("عدد البكسلات لتنعيم حافة القص فتبدو الحواف طبيعية بدل الخشنة");
    whiteBg = new QCheckBox("توحيد الخلفية إلى أبيض");
    whiteBg->setChecked(true);
    showNames = new QCheckBox("إظهار الاسم أسفل الصورة");
    showNames->setChecked(true);
    nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("اسم العميل / الصورة");
    fontSize = new QSpinBox; fontSize->setRange(8,50); fontSize->setValue(17);
    margin = new QDoubleSpinBox; margin->setRange(2,30); margin->setValue(8); margin->setSuffix(" مم");
    gap = new QDoubleSpinBox; gap->setRange(1,25); gap->setValue(4); gap->setSuffix(" مم");
    form->addRow("الورق:",paper);
    form->addRow("مقاس الصورة الثابت:",photoSizeLabel);
    auto *autoLayoutLabel = new QLabel("تخطيط تلقائي: اتجاه الورق الأفضل وعدد الخانات");
    autoLayoutLabel->setObjectName("autoLayoutLabel");
    form->addRow("",autoLayoutLabel);
    layoutStatus = new QLabel;
    layoutStatus->setObjectName("layoutStatus");
    layoutStatus->setWordWrap(true);
    form->addRow("النتيجة:",layoutStatus);
    form->addRow("الهامش:",margin);
    form->addRow("المسافة:",gap);
    s->addWidget(paperGroup);

    auto *processingGroup = new QGroupBox("تحسين الصور");
    auto *processingForm = new QFormLayout(processingGroup);
    processingForm->addRow("التحسين:",quality);
    processingForm->addRow("",removeBg);
    processingForm->addRow("حساسية الإزالة:",bgTolerance);
    processingForm->addRow("نعومة الحواف:",bgFeather);
    processingForm->addRow("",whiteBg);
    processingForm->addRow("",showNames);
    processingForm->addRow("الاسم:",nameEdit);
    processingForm->addRow("حجم الاسم:",fontSize);
    s->addWidget(processingGroup);

    auto *process = new QPushButton("تحسين ومعالجة الكل");
    process->setObjectName("primary");
    auto *reset = new QPushButton("إعادة الصور الأصلية");
    reset->setObjectName("danger");
    connect(process,&QPushButton::clicked,this,&MainWindow::processAll);
    connect(reset,&QPushButton::clicked,this,&MainWindow::resetProcessing);
    auto *processingButtons = new QHBoxLayout;
    processingButtons->addWidget(process);
    processingButtons->addWidget(reset);
    s->addLayout(processingButtons);

    auto *pdf = new QPushButton("حفظ PDF");
    pdf->setObjectName("secondary");
    auto *print = new QPushButton("طباعة مباشرة");
    print->setObjectName("primary");
    auto *imgs = new QPushButton("حفظ الصور المعالجة");
    imgs->setObjectName("secondary");
    connect(pdf,&QPushButton::clicked,this,&MainWindow::savePdf);
    connect(print,&QPushButton::clicked,this,&MainWindow::printPage);
    connect(imgs,&QPushButton::clicked,this,&MainWindow::saveImages);
    auto *outputGroup = new QGroupBox("الإخراج");
    auto *outputButtons = new QVBoxLayout(outputGroup);
    outputButtons->addWidget(pdf);
    outputButtons->addWidget(print);
    outputButtons->addWidget(imgs);
    s->addWidget(outputGroup);

    progress = new QProgressBar;
    progress->setRange(0,100);
    progress->setTextVisible(true);
    status = new QLabel("جاهز");
    status->setObjectName("status");
    status->setAlignment(Qt::AlignCenter);
    s->addWidget(progress); s->addWidget(status);

    auto *right = new QVBoxLayout;
    auto *previewFrame = new QFrame;
    previewFrame->setObjectName("previewFrame");
    auto *previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(12,12,12,12);
    previewLayout->setSpacing(8);
    auto *head = new QLabel("معاينة الطباعة");
    head->setObjectName("head");
    auto *previewHint = new QLabel("المعاينة الكاملة — الأبعاد وخطوط القص مطابقة للطباعة");
    previewHint->setObjectName("previewHint");
    previewLayout->addWidget(head);
    previewLayout->addWidget(previewHint);
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setBackgroundBrush(QColor("#dfe6f0"));
    view->setFrameShape(QFrame::StyledPanel);
    view->setFrameShadow(QFrame::Sunken);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(view,1);
    right->addWidget(previewFrame,1);

    auto *sideScroll = new QScrollArea;
    sideScroll->setObjectName("sideScroll");
    sideScroll->setWidget(side);
    sideScroll->setWidgetResizable(true);
    sideScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sideScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sideScroll->setFrameShape(QFrame::NoFrame);
    sideScroll->setMinimumWidth(390);
    sideScroll->setMaximumWidth(430);

    main->addLayout(right,1);
    main->addWidget(sideScroll);

    auto changed = [this](){ updatePreview(); };
    connect(paper,&QComboBox::currentIndexChanged,this,changed);
    connect(showNames,&QCheckBox::toggled,this,changed);
    connect(fontSize,qOverload<int>(&QSpinBox::valueChanged),this,changed);
    connect(margin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,changed);
    connect(gap,qOverload<double>(&QDoubleSpinBox::valueChanged),this,changed);
    connect(nameEdit,&QLineEdit::textChanged,this,[this](const QString &v){
        int r=list->currentRow();
        if(r>=0 && r<(int)photos.size()) {
            photos[r].name=v;
            if(list->item(r)) list->item(r)->setText(v);
        }
        updatePreview();
    });
    connect(list,&QListWidget::currentRowChanged,this,[this](int r){
        if(r>=0 && r<(int)photos.size()) nameEdit->setText(photos[r].name);
        updatePreview();
    });

    setStyleSheet(R"(
        QWidget{background:#eef2f7;color:#172033;font-family:"Segoe UI","Tahoma","Arial",sans-serif;font-size:14px;}
        QMainWindow{background:#eef2f7;}
        QFrame#side{background:#f9fbff;border:1px solid #b9c4d3;border-radius:16px;}
        QScrollArea#sideScroll{background:transparent;}
        QFrame#previewFrame{background:#ffffff;border:1px solid #aebdce;border-radius:14px;}
        QLabel#title{color:#172033;font-size:28px;font-weight:800;padding:4px 4px 0;}
        QLabel#sub{color:#526176;font-size:12px;font-weight:600;padding:0 4px 10px;}
        QLabel#listTitle{color:#172033;font-size:15px;font-weight:700;padding:6px 2px;}
        QLabel#head{color:#172033;font-size:22px;font-weight:700;padding:5px 0 8px;}
        QLabel#previewHint{color:#607089;font-size:12px;font-weight:600;padding:0 2px 5px;}
        QLabel#status{background:#f8fafc;color:#172033;border:1px solid #c4cedd;border-radius:9px;padding:10px 12px;font-weight:700;}
        QLabel#layoutStatus{background:#edf5ff;color:#164b9b;border:1px solid #a9c8f5;border-radius:8px;padding:7px;font-weight:700;}
        QLabel#fixedPhotoSize{background:#f1f4f8;color:#26364f;border:1px solid #c4cedd;border-radius:8px;padding:8px 10px;font-weight:800;}
        QGroupBox{border:1px solid #c4cedd;border-radius:10px;margin-top:9px;padding:12px 10px 10px;font-weight:700;color:#26364f;}
        QGroupBox::title{subcontrol-origin:margin;right:10px;padding:0 5px;background:#f9fbff;}
        QPushButton{background:#ffffff;color:#172033;border:1px solid #b9c4d3;border-radius:10px;min-height:42px;padding:10px 14px;font-weight:700;font-size:14px;}
        QPushButton:hover{background:#edf5ff;border-color:#7ca9f5;}
        QPushButton:pressed{background:#dfeeff;border-color:#5f8ee8;}
        QPushButton:focus{outline:none;border:2px solid #3b82f6;}
        QPushButton:disabled{background:#edf1f5;color:#7d8ca2;border-color:#d3dae5;}
        QPushButton#primary{background:#155eef;color:#ffffff;border:1px solid #0f53d2;}
        QPushButton#primary:hover{background:#0f53d2;}
        QPushButton#primary:pressed{background:#0b47b9;}
        QPushButton#secondary{background:#ffffff;color:#172033;border:1px solid #b9c4d3;}
        QPushButton#secondary:hover{background:#f3f7ff;border-color:#7ca9f5;}
        QPushButton#danger{background:#fff5f4;color:#b42318;border:1px solid #e1a9a2;}
        QPushButton#danger:hover{background:#fde8e6;border-color:#cd5b4f;}
        QPushButton#danger:pressed{background:#f7d1cc;border-color:#af3d32;}
        QListWidget{background:#ffffff;border:1px solid #b9c4d3;border-radius:10px;padding:6px;color:#172033;alternate-background-color:#f7f9fc;selection-background-color:#dfeeff;selection-color:#172033;}
        QListWidget::item{padding:8px 10px;border:1px solid transparent;border-radius:8px;}
        QListWidget::item:selected{background:#dfeeff;border:1px solid #9ebef8;}
        QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{background:#ffffff;color:#172033;padding:8px 10px;border:1px solid #b9c4d3;border-radius:8px;min-height:36px;}
        QLineEdit:focus,QComboBox:focus,QSpinBox:focus,QDoubleSpinBox:focus{border:2px solid #3b82f6;}
        QCheckBox{color:#172033;spacing:8px;font-weight:600;}
        QCheckBox::indicator{width:18px;height:18px;border:1px solid #b9c4d3;border-radius:4px;background:#ffffff;}
        QCheckBox::indicator:checked{background:#155eef;border-color:#155eef;}
        QProgressBar{border:1px solid #b9c4d3;border-radius:8px;background:#edf1f6;text-align:center;min-height:20px;}
        QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #3a85f5, stop:1 #155eef);border-radius:7px;}
        QAbstractScrollArea{border:0;}
    )");
}

void MainWindow::addImageFile(const QString &f)
{
    QImage im(f);
    if(im.isNull()) return;
    PhotoItem p{im.convertToFormat(QImage::Format_ARGB32), im.convertToFormat(QImage::Format_ARGB32),
                QFileInfo(f).completeBaseName()};
    photos.push_back(p);
    auto *item = new QListWidgetItem(QIcon(QPixmap::fromImage(im.scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation))),p.name);
    list->addItem(item);
}

void MainWindow::addPhotos()
{
    const QStringList fs = QFileDialog::getOpenFileNames(this,"اختيار الصور",QString(),
        "Images (*.jpg *.jpeg *.png *.bmp *.webp)");
    for(const QString &f: fs) addImageFile(f);
    status->setText(QString("عدد الصور: %1").arg(photos.size()));
    updatePreview();
}

void MainWindow::addFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this,"اختيار مجلد الصور");
    if(dir.isEmpty()) return;
    QDirIterator it(dir,{"*.jpg","*.jpeg","*.png","*.bmp","*.webp"},QDir::Files,QDirIterator::Subdirectories);
    int added=0;
    while(it.hasNext()) { addImageFile(it.next()); ++added; }
    status->setText(QString("تمت إضافة %1 صورة").arg(added));
    updatePreview();
}

void MainWindow::removeSelected()
{
    QList<QListWidgetItem*> sel = list->selectedItems();
    if(sel.isEmpty()) return;
    QVector<int> rows;
    for(auto *it : sel) rows.push_back(list->row(it));
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for(int r : rows){
        if(r<0 || r>=(int)photos.size()) continue;
        photos.erase(photos.begin()+r);
        delete list->takeItem(r);
    }
    status->setText(QString("عدد الصور: %1").arg(photos.size()));
    updatePreview();
}

void MainWindow::moveSelectedUp()
{
    QList<QListWidgetItem*> sel = list->selectedItems();
    if(sel.size()!=1) return;
    int r=list->row(sel.first());
    if(r<=0) return;
    std::swap(photos[r],photos[r-1]);
    auto *item=list->takeItem(r);
    list->insertItem(r-1,item);
    list->setCurrentRow(r-1);
    updatePreview();
}

void MainWindow::moveSelectedDown()
{
    QList<QListWidgetItem*> sel = list->selectedItems();
    if(sel.size()!=1) return;
    int r=list->row(sel.first());
    if(r<0 || r>=(int)photos.size()-1) return;
    std::swap(photos[r],photos[r+1]);
    auto *item=list->takeItem(r);
    list->insertItem(r+1,item);
    list->setCurrentRow(r+1);
    updatePreview();
}

void MainWindow::autoNumberNames()
{
    if(photos.empty()) return;
    QString base = nameEdit->text().trimmed();
    if(base.isEmpty()) base = "صورة";
    for(int i=0;i<(int)photos.size();++i){
        QString newName = QString("%1 %2").arg(base).arg(i+1);
        photos[i].name = newName;
        if(list->item(i)) list->item(i)->setText(newName);
    }
    status->setText("تم ترقيم الأسماء تلقائيًا");
    updatePreview();
}

void MainWindow::clearPhotos()
{
    photos.clear(); list->clear();
    status->setText("تم تفريغ الصور");
    updatePreview();
}

QImage MainWindow::removeLightBackground(const QImage &in) const
{
    QImage out=in.convertToFormat(QImage::Format_ARGB32);
    const int w=out.width(), h=out.height();
    // إزالة الخلفية الفاتحة من الحواف بطريقة flood-fill مع حد حساسية قابل للتعديل،
    // مع الحفاظ على التفاصيل الداخلية، ثم تنعيم حافة القص (feathering) لمظهر أكثر طبيعية.
    const int threshold = qBound(150, 255-bgTolerance->value(), 254);
    const int featherW = bgFeather->value();

    QVector<QPoint> q;
    QVector<uchar> removedMask(w*h,0); // 1 = خلفية تمت إزالتها
    auto isBg=[&](const QImage &img,int x,int y){
        QRgb p=img.pixel(x,y);
        return qRed(p)>threshold && qGreen(p)>threshold && qBlue(p)>threshold;
    };
    auto push=[&](int x,int y){
        if(x<0||y<0||x>=w||y>=h) return;
        int k=y*w+x; if(removedMask[k]) return;
        if(isBg(out,x,y)){removedMask[k]=1;q.push_back({x,y});}
    };
    for(int x=0;x<w;++x){push(x,0);push(x,h-1);}
    for(int y=0;y<h;++y){push(0,y);push(w-1,y);}
    for(int i=0;i<q.size();++i){
        QPoint p=q[i];
        push(p.x()+1,p.y()); push(p.x()-1,p.y()); push(p.x(),p.y()+1); push(p.x(),p.y()-1);
    }
    for(int y=0;y<h;++y)
        for(int x=0;x<w;++x)
            if(removedMask[y*w+x]) out.setPixelColor(x,y,QColor(255,255,255,0));

    if(featherW>0){
        // مسافة كل بكسل محتفظ به عن أقرب بكسل خلفية مُزالة (BFS متعدد المصادر)، ثم تدرج شفافية ناعم عند الحافة.
        QVector<int> dist(w*h,-1);
        QQueue<QPoint> bfs;
        for(int y=0;y<h;++y){
            for(int x=0;x<w;++x){
                int k=y*w+x;
                if(removedMask[k]){ dist[k]=0; bfs.enqueue({x,y}); }
            }
        }
        const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
        while(!bfs.isEmpty()){
            QPoint p=bfs.dequeue();
            int k0=p.y()*w+p.x();
            if(dist[k0]>=featherW) continue;
            for(int d=0; d<4; ++d){
                int nx=p.x()+dx[d], ny=p.y()+dy[d];
                if(nx<0||ny<0||nx>=w||ny>=h) continue;
                int k=ny*w+nx;
                if(dist[k]==-1){ dist[k]=dist[k0]+1; bfs.enqueue({nx,ny}); }
            }
        }
        for(int y=0;y<h;++y){
            for(int x=0;x<w;++x){
                int k=y*w+x;
                if(removedMask[k]) continue;
                if(dist[k]>=0 && dist[k]<featherW){
                    QColor c=out.pixelColor(x,y);
                    double t=double(dist[k])/double(featherW); // 0 عند الحافة .. 1 بعيدًا عنها
                    c.setAlphaF(qBound(0.0, t, 1.0));
                    out.setPixelColor(x,y,c);
                }
            }
        }
    }
    return out;
}

QImage MainWindow::sharpen(const QImage &in,int amount) const
{
    if(amount<=0) return in;
    QImage src=in.convertToFormat(QImage::Format_ARGB32), out=src;
    const int w=src.width(),h=src.height();
    for(int y=1;y<h-1;++y){
        for(int x=1;x<w-1;++x){
            QRgb c=src.pixel(x,y);
            QRgb l=src.pixel(x-1,y), r=src.pixel(x+1,y), u=src.pixel(x,y-1), d=src.pixel(x,y+1);
            auto calc=[&](int cc,int ll,int rr,int uu,int dd){
                int v=cc + amount*(4*cc-ll-rr-uu-dd)/10;
                return qBound(0,v,255);
            };
            out.setPixel(x,y,qRgb(calc(qRed(c),qRed(l),qRed(r),qRed(u),qRed(d)),
                                  calc(qGreen(c),qGreen(l),qGreen(r),qGreen(u),qGreen(d)),
                                  calc(qBlue(c),qBlue(l),qBlue(r),qBlue(u),qBlue(d))));
        }
    }
    return out;
}

QImage MainWindow::processImage(const QImage &in) const
{
    QImage src=in.convertToFormat(QImage::Format_ARGB32);
    int amount=quality->currentIndex()+1;
    QImage out=sharpen(src,amount);
    if(removeBg->isChecked()) out=removeLightBackground(out);
    if(whiteBg->isChecked()){
        QImage bg(out.size(),QImage::Format_ARGB32); bg.fill(Qt::white);
        QPainter p(&bg); p.drawImage(0,0,out); p.end(); out=bg;
    }
    return out;
}

void MainWindow::processAll()
{
    if(photos.empty()){QMessageBox::information(this,"تنبيه","أضف صورًا أولاً.");return;}
    progress->setValue(0);
    for(int i=0;i<(int)photos.size();++i){
        photos[i].processed=processImage(photos[i].original);
        progress->setValue((i+1)*100/photos.size());
        QApplication::processEvents();
    }
    status->setText(QString("تمت معالجة %1 صورة").arg(photos.size()));
    updatePreview();
}

void MainWindow::resetProcessing()
{
    for(auto &p:photos) p.processed=p.original;
    status->setText("تمت إعادة الصور الأصلية");
    updatePreview();
}

QSize MainWindow::paperPixels() const
{
    const QSizeF mm=paperSizeMm();
    const LayoutInfo layout=calculateLayout();
    QSize s(qRound(mm.width()/25.4*300.0),qRound(mm.height()/25.4*300.0));
    if(layout.landscape) s.transpose();
    return s;
}

QSizeF MainWindow::selectedPhotoSize() const
{
    return QSizeF(4.0,6.0);
}

QSizeF MainWindow::paperSizeMm() const
{
    switch(paper->currentIndex()){
    case 1: return QSizeF(148.0,210.0);
    case 2: return QSizeF(100.0,150.0);
    case 3: return QSizeF(215.9,279.4);
    default: return QSizeF(210.0,297.0);
    }
}

MainWindow::LayoutInfo MainWindow::calculateLayout() const
{
    const QSizeF paperMm=paperSizeMm();
    const QSizeF portraitCm=QSizeF(paperMm.width()/10.0,paperMm.height()/10.0);
    const double m=margin->value()/10.0, g=gap->value()/10.0;
    const QSizeF requested=selectedPhotoSize();
    auto count=[&](const QSizeF &page){
        return qMakePair(qMax(0,int(qFloor((page.width()-2*m+g)/(requested.width()+g)))),
                         qMax(0,int(qFloor((page.height()-2*m+g)/(requested.height()+g)))));
    };
    const auto portrait=count(portraitCm);
    const auto landscape=count(QSizeF(portraitCm.height(),portraitCm.width()));
    const bool useLandscape=landscape.first*landscape.second > portrait.first*portrait.second;
    const auto chosen=useLandscape?landscape:portrait;
    LayoutInfo best;
    best.columns=chosen.first;
    best.rows=chosen.second;
    best.photoCm=requested;
    best.landscape=useLandscape;
    return best;
}

void MainWindow::rebuildPage(QPainter *external,const QRectF &target)
{
    QSize ps=paperPixels();
    QImage page(ps,QImage::Format_RGB32); page.fill(Qt::white);
    QPainter p(&page);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const LayoutInfo layout=calculateLayout();
    const double scale=300.0/2.54;
    const double m=margin->value()/25.4*300.0, g=gap->value()/25.4*300.0;
    const double pw=layout.photoCm.width()*scale, ph=layout.photoCm.height()*scale;
    const int c=layout.columns, r=layout.rows;
    const double cw=pw;
    const double ch=ph;
    const int per=c*r;
    const int slotCount=photos.empty()?0:per;
    for(int i=0;i<slotCount;++i){
        int rr=i/c, cc=i%c;
        double x=ps.width()-m-(cc+1)*cw-cc*g; // RTL
        double y=m+rr*(ch+g);
        QRectF photoRect(x,y,cw,ch);
        const int photoIndex=i%static_cast<int>(photos.size());
        QImage im=photos[photoIndex].processed;
        if(im.isNull()) im=photos[photoIndex].original;
        QImage fit=im.scaled(photoRect.size().toSize(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QRectF ir(photoRect.x()+(photoRect.width()-fit.width())/2,
                  photoRect.y()+(photoRect.height()-fit.height())/2,
                  fit.width(),fit.height());
        p.drawImage(ir,fit);
        // Dashed cutting guides follow the physical photo rectangle, not the image content.
        p.save();
        QPen cutPen(QColor("#8b95a3"), 2, Qt::DashLine);
        cutPen.setDashPattern({7.0, 5.0});
        p.setPen(cutPen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(photoRect);
        p.restore();
        if(showNames->isChecked()){
            QFont f("Arial"); f.setBold(true); f.setPixelSize(fontSize->value()*300/72);
            p.setFont(f); p.setPen(Qt::black);
            const QRectF labelRect=photoRect.adjusted(3,photoRect.height()-f.pixelSize()-8,-3,-3);
            p.fillRect(labelRect,QColor(255,255,255,205));
            p.drawText(labelRect,Qt::AlignCenter|Qt::TextWordWrap,photos[photoIndex].name);
        }
    }
    p.end();

    if(external){
        external->setRenderHint(QPainter::SmoothPixmapTransform);
        external->drawImage(target,page);
    } else {
        scene->clear();
        scene->setSceneRect(0,0,ps.width(),ps.height());
        scene->addPixmap(QPixmap::fromImage(page));
        view->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);
    }
}

void MainWindow::updatePreview()
{
    const LayoutInfo layout=calculateLayout();
    const QString paperName=paper->currentText();
    const QString sizeName="4 × 6 سم";
    const int totalSlots=layout.columns*layout.rows;
    const int repeats=photos.empty()?0:qMax(0,totalSlots-static_cast<int>(photos.size()));
    const QString orientation=layout.landscape?"أفقي":"رأسي";
    layoutStatus->setText(QString("%1 %2: %3 أعمدة × %4 صفوف = %5 صور مقاس %6")
                          .arg(paperName).arg(orientation).arg(layout.columns).arg(layout.rows)
                          .arg(totalSlots).arg(sizeName)
                          + (repeats>0
                             ? QString(" — تكرار %1 نسخة لملء الورقة").arg(repeats)
                             : QString()));
    rebuildPage();
}

void MainWindow::savePdf()
{
    if(photos.empty()){QMessageBox::information(this,"تنبيه","أضف صورًا أولاً.");return;}
    QString f=QFileDialog::getSaveFileName(this,"حفظ PDF",QDir::homePath()+"/صور_المعاملات.pdf","PDF (*.pdf)");
    if(f.isEmpty()) return;
    QPdfWriter pdf(f); pdf.setResolution(300);
    const LayoutInfo layout=calculateLayout();
    const QPageSize::PageSizeId id=paper->currentIndex()==0?QPageSize::A4:
        paper->currentIndex()==1?QPageSize::A5:QPageSize::Custom;
    pdf.setPageSize(id==QPageSize::Custom
                    ? QPageSize(QSizeF(paperSizeMm().width(),paperSizeMm().height()),QPageSize::Millimeter)
                    : QPageSize(id));
    pdf.setPageOrientation(layout.landscape?QPageLayout::Landscape:QPageLayout::Portrait);
    QRect target=pdf.pageLayout().paintRectPixels(pdf.resolution());
    QPainter p(&pdf);
    rebuildPage(&p,QRectF(target));
    p.end();
    status->setText("تم حفظ PDF بجودة 300 DPI");
}

void MainWindow::printPage()
{
    if(photos.empty()){QMessageBox::information(this,"تنبيه","أضف صورًا أولاً.");return;}
    QPrinter printer(QPrinter::HighResolution);
    const LayoutInfo layout=calculateLayout();
    const QPageSize::PageSizeId id=paper->currentIndex()==0?QPageSize::A4:
        paper->currentIndex()==1?QPageSize::A5:QPageSize::Custom;
    printer.setPageSize(id==QPageSize::Custom
                        ? QPageSize(QSizeF(paperSizeMm().width(),paperSizeMm().height()),QPageSize::Millimeter)
                        : QPageSize(id));
    printer.setPageOrientation(layout.landscape?QPageLayout::Landscape:QPageLayout::Portrait);
    QPrintDialog dlg(&printer,this);
    if(dlg.exec()!=QDialog::Accepted) return;
    QRect target=printer.pageLayout().paintRectPixels(printer.resolution());
    QPainter p(&printer);
    rebuildPage(&p,QRectF(target));
    p.end();
    status->setText("تم إرسال الصفحة إلى الطابعة");
}

void MainWindow::saveImages()
{
    if(photos.empty()){QMessageBox::information(this,"تنبيه","أضف صورًا أولاً.");return;}
    QString dir=QFileDialog::getExistingDirectory(this,"اختيار مجلد الحفظ");
    if(dir.isEmpty()) return;
    int n=0;
    for(const auto &p:photos){
        QString safe=p.name;
        for(QChar c:QString("\\/:*?\"<>|")) safe.replace(c,'_');
        QString file=QDir(dir).filePath(safe+".png");
        if(p.processed.save(file,"PNG")) ++n;
    }
    status->setText(QString("تم حفظ %1 صورة").arg(n));
}
