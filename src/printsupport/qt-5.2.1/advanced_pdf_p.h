/**
 * This file is part of OpenOrienteering.
 *
 * This is a modified version of a file from the Qt Toolkit.
 * You can redistribute it and/or modify it under the terms of
 * the GNU General Public License, version 3, as published by
 * the Free Software Foundation.
 *
 * OpenOrienteering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
 *
 * Changes:
 * 2015-10-18 Kai Pastor <dg0yt@darc.de>
 * - Adjustment of legal information
 * - Modifications required for separate compilation:
 *   - Renaming of selected files, classes, members and macros
 *   - Adjustment of include statements
 *   - Removal of Q_XXX_EXPORT
 *   - Distinct paint engine type
 */
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
****************************************************************************/

#ifndef ADVANCED_PDF_P_H
#define ADVANCED_PDF_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifndef QT_NO_PDF

#include "QtGui/qmatrix.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"
#include <private/qstroker_p.h>
#include <private/qpaintengine_p.h>
#include <private/qfontengine_p.h>
#include "private/qfontsubset_p.h"

// #define USE_NATIVE_GRADIENTS

QT_BEGIN_NAMESPACE

const char *qt_real_to_string(qreal val, char *buf);
const char *qt_int_to_string(int val, char *buf);

namespace AdvancedPdf {

    class ByteStream
    {
    public:
        // fileBacking means that ByteStream will buffer the contents on disk
        // if the size exceeds a certain threshold. In this case, if a byte
        // array was passed in, its contents may no longer correspond to the
        // ByteStream contents.
        explicit ByteStream(bool fileBacking = false);
        explicit ByteStream(QByteArray *ba, bool fileBacking = false);
        ~ByteStream();
        ByteStream &operator <<(char chr);
        ByteStream &operator <<(const char *str);
        ByteStream &operator <<(const QByteArray &str);
        ByteStream &operator <<(const ByteStream &src);
        ByteStream &operator <<(qreal val);
        ByteStream &operator <<(int val);
        ByteStream &operator <<(const QPointF &p);
        // Note that the stream may be invalidated by calls that insert data.
        QIODevice *stream();
        void clear();

        static inline int maxMemorySize() { return 100000000; }
        static inline int chunkSize()     { return 10000000; }

    protected:
        void constructor_helper(QIODevice *dev);
        void constructor_helper(QByteArray *ba);

    private:
        void prepareBuffer();

    private:
        QIODevice *dev;
        QByteArray ba;
        bool fileBackingEnabled;
        bool fileBackingActive;
        bool handleDirty;
    };

    enum PathFlags {
        ClipPath,
        FillPath,
        StrokePath,
        FillAndStrokePath
    };
    QByteArray generatePath(const QPainterPath &path, const QTransform &matrix, PathFlags flags);
    QByteArray generateMatrix(const QTransform &matrix);
    QByteArray generateDashes(const QPen &pen);
    QByteArray patternForBrush(const QBrush &b);
#ifdef USE_NATIVE_GRADIENTS
    QByteArray generateLinearGradientShader(const QLinearGradient *lg, const QPointF *page_rect, bool alpha = false);
#endif

    struct Stroker {
        Stroker();
        void setPen(const QPen &pen, QPainter::RenderHints hints);
        void strokePath(const QPainterPath &path);
        ByteStream *stream;
        bool first;
        QTransform matrix;
        bool cosmeticPen;
    private:
        QStroker basicStroker;
        QDashStroker dashStroker;
        QStrokerOps *stroker;
    };

    QByteArray ascii85Encode(const QByteArray &input);

    const char *toHex(ushort u, char *buffer);
    const char *toHex(uchar u, char *buffer);

}


class AdvancedPdfPage : public AdvancedPdf::ByteStream
{
public:
    AdvancedPdfPage();

    QVector<uint> images;
    QVector<uint> graphicStates;
    QVector<uint> patterns;
    QVector<uint> fonts;
    QVector<uint> annotations;

    void streamImage(int w, int h, int object);

    QSize pageSize;
private:
};

class AdvancedPdfWriter;
class AdvancedPdfEnginePrivate;

class AdvancedPdfEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(AdvancedPdfEngine)
    friend class AdvancedPdfWriter;
public:
    static struct PaintEngineTypeStruct
    {
        constexpr operator QPaintEngine::Type() const {
            return QPaintEngine::Type(QPaintEngine::User + 1);
        }
    } PaintEngineType;

    AdvancedPdfEngine();
    AdvancedPdfEngine(AdvancedPdfEnginePrivate &d);
    ~AdvancedPdfEngine() {}

    void setOutputFilename(const QString &filename);
    inline void setResolution(int resolution);

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev);
    bool end();

    void drawPoints(const QPointF *points, int pointCount);
    void drawLines(const QLineF *lines, int lineCount);
    void drawRects(const QRectF *rects, int rectCount);
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void drawPath (const QPainterPath & path);

    void drawTextItem(const QPointF &p, const QTextItem &textItem);

    void drawPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QRectF & sr);
    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);
    void drawTiledPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QPointF & point);

    void updateState(const QPaintEngineState &state);

    int metric(QPaintDevice::PaintDeviceMetric metricType) const;
    Type type() const;
    // end reimplementations QPaintEngine

    // Printer stuff...
    bool newPage();

    void setPen();
    void setBrush();
    void setupGraphicsState(QPaintEngine::DirtyFlags flags);

private:
    void updateClipPath(const QPainterPath & path, Qt::ClipOperation op);
};

class AdvancedPdfEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(AdvancedPdfEngine)
public:
    AdvancedPdfEnginePrivate();
    ~AdvancedPdfEnginePrivate();

    inline uint requestObject() { return currentObject++; }

    QRect paperRect() const;
    QRect pageRect() const;
    void setPaperSize(const QSizeF &pageSizeMM);

    int width() const {
        QRect r = paperRect();
        return qRound(r.width()*72./resolution);
    }
    int height() const {
        QRect r = paperRect();
        return qRound(r.height()*72./resolution);
    }

    void writeHeader();
    void writeTail();

    int addImage(const QImage &image, bool *bitmap, qint64 serial_no);
    int addConstantAlphaObject(int brushAlpha, int penAlpha = 255);
    int addBrushPattern(const QTransform &matrix, bool *specifyColor, int *gStateObject);

    void drawTextItem(const QPointF &p, const QTextItemInt &ti);

    QTransform pageMatrix() const;

    void newPage();

    int currentObject;

    AdvancedPdfPage* currentPage;
    AdvancedPdf::Stroker stroker;

    QPointF brushOrigin;
    QBrush brush;
    QPen pen;
    QList<QPainterPath> clips;
    bool clipEnabled;
    bool allClipped;
    bool hasPen;
    bool hasBrush;
    bool simplePen;
    qreal opacity;

    QHash<QFontEngine::FaceId, QFontSubset *> fonts;

    QPaintDevice *pdev;

    // the device the output is in the end streamed to.
    QIODevice *outDevice;
    bool ownsDevice;

    // printer options
    QString outputFileName;
    QString title;
    QString creator;
    bool fullPage;
    bool embedFonts;
    int resolution;
    bool landscape;
    bool grayscale;

    // in postscript points
    QSizeF paperSize;
    qreal leftMargin, topMargin, rightMargin, bottomMargin;

private:
#ifdef USE_NATIVE_GRADIENTS
    int gradientBrush(const QBrush &b, const QMatrix &matrix, int *gStateObject);
#endif

    void writeInfo();
    void writePageRoot();
    void writeFonts();
    void embedFont(QFontSubset *font);

    QVector<int> xrefPositions;
    QDataStream* stream;
    int streampos;

    int writeImage(const QByteArray &data, int width, int height, int depth,
                   int maskObject, int softMaskObject, bool dct = false);
    void writePage();

    int addXrefEntry(int object, bool printostr = true);
    void printString(const QString &string);
    void xprintf(const char* fmt, ...);
    inline void write(const QByteArray &data) {
        stream->writeRawData(data.constData(), data.size());
        streampos += data.size();
    }

    int writeCompressed(const char *src, int len);
    inline int writeCompressed(const QByteArray &data) { return writeCompressed(data.constData(), data.length()); }
    int writeCompressed(QIODevice *dev);

    // various PDF objects
    int pageRoot, catalog, info, graphicsState, patternColorSpace;
    QVector<uint> pages;
    QHash<qint64, uint> imageCache;
    QHash<QPair<uint, uint>, uint > alphaCache;
};

void AdvancedPdfEngine::setResolution(int resolution)
{
    Q_D(AdvancedPdfEngine);
    d->resolution = resolution;
}

QT_END_NAMESPACE

#endif // QT_NO_PDF

#endif // ADVANCED_PDF_P_H

