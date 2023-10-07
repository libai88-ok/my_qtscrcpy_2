/*float vertices[] = {
    //绘制矩形的顶点坐标     //纹理顶点坐标
    1.0f, 1.0f, 0.0f,       1.0f, 1.0f,          // 右上角
    -1.0f, 1.0f, 0.0f,      0.0f, 1.0f,   // 左上角
    1.0f, -1.0f, 0.0f,      1.0f, 0.0f,    // 右下角
    -1.0f, -1.0f, 0.0f,     0.0f, 0.0f        // 左下角
};

unsigned int indices[] = {
    // 注意索引从0开始!
    // 此例的索引(0,1,2,3)就是顶点数组vertices的下标，
    // 这样可以由下标代表顶点组合成矩形

    0, 1, 3, // 第一个三角形
    1, 2, 3  // 第二个三角形
};


QYUVOpenGLWidget::QYUVOpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{

}
// layout (location = 0)；定义顶点位置标识id，与glVertexAttribPointer配合使用
//in输入，一个vec3：三元素向量 aPos（变量名称）
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord; \n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "   TexCoord = aTexCoord;"
    "}\0";

//out 输出 vec4：红色、绿色、蓝色和alpha(透明度)分量
const char *fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        "    FragColor = texture(ourTexture, TexCoord);\n"//vec4(0.0f, 1.0f, 0.0f, 1.0f);\n"//
        "} \0";

void QYUVOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    //设置背景
    glClearColor(255, 0, 0, 0);

    //顶点缓存对象。从CPU把数据发送到显卡相对较慢，可以将一大批数据送到显卡，提高顶点数据传送速率

    //定义顶点缓存对象id
    unsigned int VBO;
    //生成顶点缓存对象：对象数量、id
    glGenBuffers(1, &VBO);
    //绑定顶点缓冲对象到opengl，传给opengl;从这一刻起，我们使用的任何（在GL_ARRAY_BUFFER目标上的）缓冲调用都会用来配置当前绑定的缓冲(VBO)。
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //glBufferData是一个专门用来把用户定义的数据复制到当前绑定缓冲的函数。
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    //创建两个三角形, EBO是一个缓冲区，元素缓冲区对象，就像一个顶点缓冲区对象一样
    //它存储 OpenGL 用来决定要绘制哪些顶点的索引。这种所谓的索引绘制(Indexed Drawing)正是我们问题的解决方案。
    /*unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //顶点着色器，数据从缓冲器到显卡；控制绘制图形的位置

    //定义id，创建顶点着色器
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    //指定顶点着色器的源码
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    //编译顶点着色器
    glCompileShader(vertexShader);

    //片段着色器,控制绘制图形的颜色
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    //着色器程序，用于连接着色器
    //创建
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    //将着色器附加到程序上
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    //连接
    glLinkProgram(shaderProgram);
    //激活
    glUseProgram(shaderProgram);
    //将着色器附加到程序之后，就可以删除这些着色器了
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //纹理贴图
    //定义纹理id

    //生成纹理对象
    glGenTextures(1, &texture);
    //绑定纹理GL_TEXTURE_2D类型
    glBindTexture(GL_TEXTURE_2D, texture);
    // 为当前绑定的纹理对象设置环绕、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //qimage加载图片
    QImage imag;
    imag.load("C:\\Users\\13503\\Desktop\\1.png");
    imag = QGLWidget::convertToGLFormat(imag);
    //图片填充纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imag.width(), imag.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, imag.bits());
    glGenerateMipmap(GL_TEXTURE_2D);

    //顶点数组访问方式;参数一：location；二：顶点数目；三：顶点元素数据类型；四：是否标准化；五：步长；六：标准化
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    //启用顶点数组
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    //启动纹理顶点数组
    glEnableVertexAttribArray(1);

}

void QYUVOpenGLWidget::resizeGL(int w, int h)
{

}

void QYUVOpenGLWidget::paintGL()
{
    glBindTexture(GL_TEXTURE_2D, texture);
    //glBindVertexArray(VAO);
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //用于绘制VBO第一个参数：绘制参数的类型；二：指定顶点数组的起始顶点；三：一共绘制多少个顶点
    //参数GL_TRIANGLE_STRIP，按照步长绘制三角形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //用于绘制EBO，第一参数，类型；二：顶点数目；三，索引的类型；四，起始便宜量
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}*/

