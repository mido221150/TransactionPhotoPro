#include "MainWindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QFrame>
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
    paper->setCurrentIndex(s.value("paper",0).toInt());
    columns->setValue(s.value("columns",2).toInt());
    rows->setValue(s.value("rows",2).toInt());
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
    s.setValue("columns",columns->value());
    s.setValue("rows",rows->value());
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
    side->setMinimumWidth(365);
    auto *s = new QVBoxLayout(side);
    s->setSpacing(8);

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
    s->addWidget(add); s->addWidget(folder);
    s->addWidget(del); s->addWidget(clear);

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
    s->addLayout(moveRow);
    s->addWidget(autoNum);

    auto *form = new QFormLayout;
    paper = new QComboBox;
    paper->addItems({"A4 رأسي","A4 أفقي","A5 رأسي","A5 أفقي"});
    columns = new QSpinBox; columns->setRange(1,8); columns->setValue(2);
    rows = new QSpinBox; rows->setRange(1,12); rows->setValue(2);
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
    form->addRow("الأعمدة:",columns);
    form->addRow("الصفوف:",rows);
    form->addRow("التحسين:",quality);
    form->addRow("",removeBg);
    form->addRow("حساسية الإزالة:",bgTolerance);
    form->addRow("نعومة الحواف:",bgFeather);
    form->addRow("",whiteBg);
    form->addRow("",showNames);
    form->addRow("الاسم:",nameEdit);
    form->addRow("حجم الاسم:",fontSize);
    form->addRow("الهامش:",margin);
    form->addRow("المسافة:",gap);
    s->addLayout(form);

    auto *process = new QPushButton("تحسين ومعالجة الكل");
    process->setObjectName("primary");
    auto *reset = new QPushButton("إعادة الصور الأصلية");
    reset->setObjectName("danger");
    connect(process,&QPushButton::clicked,this,&MainWindow::processAll);
    connect(reset,&QPushButton::clicked,this,&MainWindow::resetProcessing);
    s->addWidget(process); s->addWidget(reset);

    auto *pdf = new QPushButton("حفظ PDF");
    pdf->setObjectName("secondary");
    auto *print = new QPushButton("طباعة مباشرة");
    print->setObjectName("primary");
    auto *imgs = new QPushButton("حفظ الصور المعالجة");
    imgs->setObjectName("secondary");
    connect(pdf,&QPushButton::clicked,this,&MainWindow::savePdf);
    connect(print,&QPushButton::clicked,this,&MainWindow::printPage);
    connect(imgs,&QPushButton::clicked,this,&MainWindow::saveImages);
    s->addWidget(pdf); s->addWidget(print); s->addWidget(imgs);

    progress = new QProgressBar;
    progress->setRange(0,100);
    progress->setTextVisible(true);
    status = new QLabel("جاهز");
    status->setObjectName("status");
    status->setAlignment(Qt::AlignCenter);
    s->addWidget(progress); s->addWidget(status);

    auto *right = new QVBoxLayout;
    auto *head = new QLabel("معاينة الطباعة");
    head->setObjectName("head");
    right->addWidget(head);
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setBackgroundBrush(QColor("#dfe6f0"));
    view->setFrameShape(QFrame::StyledPanel);
    view->setFrameShadow(QFrame::Sunken);
    right->addWidget(view,1);

    main->addLayout(right,1);
    main->addWidget(side);

    auto changed = [this](){ updatePreview(); };
    connect(paper,&QComboBox::currentIndexChanged,this,changed);
    connect(columns,qOverload<int>(&QSpinBox::valueChanged),this,changed);
    connect(rows,qOverload<int>(&QSpinBox::valueChanged),this,changed);
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
        QLabel#title{color:#172033;font-size:28px;font-weight:800;padding:4px 4px 0;}
        QLabel#sub{color:#526176;font-size:12px;font-weight:600;padding:0 4px 10px;}
        QLabel#listTitle{color:#172033;font-size:15px;font-weight:700;padding:6px 2px;}
        QLabel#head{color:#172033;font-size:22px;font-weight:700;padding:5px 0 8px;}
        QLabel#status{background:#f8fafc;color:#172033;border:1px solid #c4cedd;border-radius:9px;padding:10px 12px;font-weight:700;}
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
    bool a4=paper->currentIndex()<2;
    bool landscape=paper->currentIndex()%2==1;
    QSize s=a4?QSize(2480,3508):QSize(1748,2480); // 300 DPI تقريبًا
    if(landscape) s.transpose();
    return s;
}

void MainWindow::rebuildPage(QPainter *external,const QRectF &target)
{
    QSize ps=paperPixels();
    QImage page(ps,QImage::Format_RGB32); page.fill(Qt::white);
    QPainter p(&page);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    double m=margin->value()/25.4*300.0, g=gap->value()/25.4*300.0;
    int c=columns->value(), r=rows->value();
    double cw=(ps.width()-2*m-(c-1)*g)/c;
    double ch=(ps.height()-2*m-(r-1)*g)/r;
    int per=c*r;
    for(int i=0;i<std::min(per,(int)photos.size());++i){
        int rr=i/c, cc=i%c;
        double x=ps.width()-m-(cc+1)*cw-cc*g; // RTL
        double y=m+rr*(ch+g);
        double nh=showNames->isChecked()?fontSize->value()*300.0/72.0*1.8:0;
        QRectF ib(x+8,y+8,cw-16,ch-nh-12);
        QImage im=photos[i].processed;
        if(im.isNull()) im=photos[i].original;
        QImage fit=im.scaled(ib.size().toSize(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QRectF ir(x+(cw-fit.width())/2,y+(ch-nh-fit.height())/2,fit.width(),fit.height());
        p.drawImage(ir,fit);
        p.setPen(QPen(QColor("#d5d9e0"),2)); p.drawRect(QRectF(x,y,cw,ch));
        if(showNames->isChecked()){
            QFont f("Arial"); f.setBold(true); f.setPixelSize(fontSize->value()*300/72);
            p.setFont(f); p.setPen(Qt::black);
            p.drawText(QRectF(x+5,y+ch-nh,cw-10,nh),Qt::AlignCenter|Qt::TextWordWrap,photos[i].name);
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

void MainWindow::updatePreview(){ rebuildPage(); }

void MainWindow::savePdf()
{
    if(photos.empty()){QMessageBox::information(this,"تنبيه","أضف صورًا أولاً.");return;}
    QString f=QFileDialog::getSaveFileName(this,"حفظ PDF",QDir::homePath()+"/صور_المعاملات.pdf","PDF (*.pdf)");
    if(f.isEmpty()) return;
    QPdfWriter pdf(f); pdf.setResolution(300);
    QPageSize::PageSizeId id=(paper->currentIndex()<2)?QPageSize::A4:QPageSize::A5;
    pdf.setPageSize(QPageSize(id));
    pdf.setPageOrientation(paper->currentIndex()%2?QPageLayout::Landscape:QPageLayout::Portrait);
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
    printer.setPageSize(paper->currentIndex()<2?QPageSize(QPageSize::A4):QPageSize(QPageSize::A5));
    printer.setPageOrientation(paper->currentIndex()%2?QPageLayout::Landscape:QPageLayout::Portrait);
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

