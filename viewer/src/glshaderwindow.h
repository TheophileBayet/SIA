#ifndef GLSHADERWINDOW_H
#define GLSHADERWINDOW_H

#include "openglwindow.h"
#include "TriMesh.h"
#include "joint.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector4D>
#include <QtGui/QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QScreen>
#include <QMouseEvent>


class glShaderWindow : public OpenGLWindow
{
    Q_OBJECT
public:
    glShaderWindow(QWindow *parent = 0);
    ~glShaderWindow();

    void initialize();
    void render();
    void resize(int x, int y);
    void setWorkingDirectory(QString& myPath, QString& myName, QString& texture, QString& envMap);
    inline const QString& getWorkingDirectory() { return workingDirectory;};
    inline const QStringList& fragShaderSuffix() { return m_fragShaderSuffix;};
    inline const QStringList& vertShaderSuffix() { return m_vertShaderSuffix;};

public slots:
    void openSceneFromFile();
    void openNewTexture();
    void openNewEnvMap();
    void saveScene();
    void toggleFullScreen();
    void saveScreenshot();
    QWidget* makeAuxWindow();
    void setWindowSize(const QString& size);
    void setShader(const QString& size);
    void cookTorranceClicked();
    void blinnPhongClicked();
    void transparentClicked();
    void bubbleClicked();
    void opaqueClicked();
    void directClicked();
    void indirectClicked();
    void updateLightIntensity(int lightSliderValue);
    void updateShininess(int shininessSliderValue);
    void updateEtaReal(int etaSliderValue);
    void updateEtaImaginary(int etaSliderValue);
    void updateRefractions(int refractionsSliderValue);
    void updateInnerRadius(int innerRadiusSliderValue);
    void updateAnimating();
    void updateJoints(Joint* root);

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent *e);
    void resizeEvent(QResizeEvent * ev);
    void wheelEvent(QWheelEvent * ev);
    void keyPressEvent(QKeyEvent* e);

private:
    QOpenGLShaderProgram* prepareShaderProgram(const QString& vertexShaderPath, const QString& fragmentShaderPath);
    QOpenGLShaderProgram* prepareComputeProgram(const QString& computeShaderPath);
    void createSSBO();
    void bindSceneToProgram();
    void initializeTransformForScene();
    void initPermTexture();
    void loadTexturesForShaders();
    void openScene();
    void mouseToTrackball(QVector2D &in, QVector3D &out);
    int treeCount(Joint* root, int count);
    void treeConstruct(Joint* root);
    // Are we using GPGPU?
    bool isGPGPU;
    // Do we change the shader?
    bool changeShader;
    // Did we use the wheel ?
    bool usedWheel;
    // Are we animating ?
    bool animating;
    // which pixel should we treat when animating ?
    bool animation_time;
    // Are we using fullrt?
    bool isFullrt;
    // Are we animating the skeleton ?
    bool isSkeleton;
    // Are we using compute shaders?
    bool hasComputeShaders;
    // Model we are displaying:
    QString  workingDirectory;
    QString  modelName;
    QString  textureName;
    QString  envMapName;
    trimesh::TriMesh* modelMesh;
    uchar* pixels;
    // Ground
    trimesh::point *g_vertices;
    trimesh::vec *g_normals;
    trimesh::vec2 *g_texcoords;
    trimesh::point *g_colors;
    int *g_indices;
    int g_numPoints;
    int g_numIndices;
    // DEUXIEME GROUND2
    trimesh::point *g2_vertices;
    trimesh::vec *g2_normals;
    trimesh::vec2 *g2_texcoords;
    trimesh::point *g2_colors;
    int *g2_indices;
    int g2_numPoints;
    int g2_numIndices;
    Joint* root;
    int frame;
    trimesh::point *trn ;
    // Joints
    //TODO : adapter pour la lecture des joinst
    trimesh::point *j_vertices;
    trimesh::point *j_colors;
    int *j_indices;
    int j_numPoints;
    int j_numIndices;
    // GPGPU
    trimesh::point *gpgpu_vertices;
    trimesh::vec *gpgpu_normals;
    trimesh::vec2 *gpgpu_texcoords;
    trimesh::point *gpgpu_colors;
    int *gpgpu_indices;
    int compute_groupsize_x;
    int compute_groupsize_y;
    // ComputeShader:
    GLuint ssbo[4];
    // Parameters controlled by UI
    bool blinnPhong;
    bool transparent;
    bool lightning;
    bool bubble;
    int refractions;
    float innerRadius;
    QVector2D eta;
    float lightIntensity;
    float shininess;
    float lightDistance;
    float groundDistance;


    // OpenGL variables encapsulated by Qt
    QOpenGLShaderProgram *m_program;
    QOpenGLShaderProgram *ground_program;
    QOpenGLShaderProgram *ground2_program;
    QOpenGLShaderProgram *compute_program;
    QOpenGLShaderProgram *shadowMapGenerationProgram;
    QOpenGLShaderProgram *joints_program;
    QOpenGLTexture* environmentMap;
    QOpenGLTexture* texture;
    QOpenGLTexture* permTexture;   // for Perlin noise
    QOpenGLTexture* computeResult; // output of compute shader
    // Model
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLBuffer m_normalBuffer;
    QOpenGLBuffer m_colorBuffer;
    QOpenGLBuffer m_texcoordBuffer;
    QOpenGLVertexArrayObject m_vao;
    int m_numFaces;
	QVector3D m_center;
	QVector3D m_bbmin;
	QVector3D m_bbmax;
    // Ground
    QOpenGLVertexArrayObject ground_vao;
    QOpenGLBuffer ground_vertexBuffer;
    QOpenGLBuffer ground_indexBuffer;
    QOpenGLBuffer ground_normalBuffer;
    QOpenGLBuffer ground_colorBuffer;
    QOpenGLBuffer ground_texcoordBuffer;
    // DEUXIEME ground2
    QOpenGLVertexArrayObject ground2_vao;
    QOpenGLBuffer ground2_vertexBuffer;
    QOpenGLBuffer ground2_indexBuffer;
    QOpenGLBuffer ground2_normalBuffer;
    QOpenGLBuffer ground2_colorBuffer;
    QOpenGLBuffer ground2_texcoordBuffer;
    // Joints
    QOpenGLVertexArrayObject joints_vao;
    QOpenGLBuffer joints_vertexBuffer;
    QOpenGLBuffer joints_indexBuffer;
    QOpenGLBuffer joints_normalBuffer;
    QOpenGLBuffer joints_colorBuffer;
    QOpenGLBuffer joints_texcoordBuffer;
    // Matrix for all objects
    QMatrix4x4 m_matrix[3]; // 0 = object, 1 = light, 2 = ground
    QMatrix4x4 m_perspective;
    // Shadow mapping
    GLuint shadowMap_fboId;
    GLuint shadowMap_rboId;
    GLuint shadowMap_textureId;
    int shadowMapDimension;
    // User interface variables
    bool fullScreenSnapshots;
    QStringList m_fragShaderSuffix;
    QStringList m_vertShaderSuffix;
    QStringList m_compShaderSuffix;
    QVector2D lastMousePosition;
    QVector3D lastTBPosition;
    Qt::MouseButton mouseButton;
    float m_screenSize; // max window dimension
    QWidget* auxWidget; // window for parameters
    QWidget* container;
};

#endif // GLSHADERWINDOW_H
