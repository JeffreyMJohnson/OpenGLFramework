#include "framework.h"



GLuint GLF::CreateShader(GLenum a_eShaderType, const char *a_strShaderFile)
{
	std::string strShaderCode;
	//open shader file
	std::ifstream shaderStream(a_strShaderFile);
	//if that worked ok, load file line by line
	if (shaderStream.is_open())
	{
		std::string Line = "";
		while (std::getline(shaderStream, Line))
		{
			strShaderCode += "\n" + Line;
		}
		shaderStream.close();
	}

	//convert to cstring
	char const *szShaderSourcePointer = strShaderCode.c_str();

	//create shader ID
	GLuint uiShader = glCreateShader(a_eShaderType);
	//load source code
	glShaderSource(uiShader, 1, &szShaderSourcePointer, NULL);

	//compile shader
	glCompileShader(uiShader);

	//check for compilation errors and output them
	GLint iStatus;
	glGetShaderiv(uiShader, GL_COMPILE_STATUS, &iStatus);
	if (iStatus == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(uiShader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(uiShader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (a_eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return uiShader;
}

GLuint GLF::CreateProgram(const char *a_vertex, const char *a_frag)
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(CreateShader(GL_VERTEX_SHADER, a_vertex));
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, a_frag));

	//create shader program ID
	GLuint uiProgram = glCreateProgram();

	//attach shaders
	for (auto shader = shaderList.begin(); shader != shaderList.end(); shader++)
		glAttachShader(uiProgram, *shader);

	//link program
	glLinkProgram(uiProgram);

	//check for link errors and output them
	GLint status;
	glGetProgramiv(uiProgram, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(uiProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(uiProgram, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (auto shader = shaderList.begin(); shader != shaderList.end(); shader++)
	{
		glDetachShader(uiProgram, *shader);
		glDeleteShader(*shader);
	}

	return uiProgram;
}

const float* GLF::getOrtho(float left, float right, float bottom, float top, float a_fNear, float a_fFar)
{
	//to correspond with mat4 in the shader
	float * toReturn = new float[12];
	toReturn[0] = 2.0 / (right - left);
	toReturn[1] = toReturn[2] = toReturn[3] = toReturn[4] = 0;
	toReturn[5] = 2.0 / (top - bottom);
	toReturn[6] = toReturn[7] = toReturn[8] = toReturn[9] = 0;
	toReturn[10] = 2.0 / (a_fFar - a_fNear);
	toReturn[11] = 0;
	toReturn[12] = -1 * ((right + left) / (right - left));
	toReturn[13] = -1 * ((top + bottom) / (top - bottom));
	toReturn[14] = -1 * ((a_fFar + a_fNear) / (a_fFar - a_fNear));
	toReturn[15] = 1;
	return toReturn;
}

void GLF::DrawSprite(Sprite& s_object)
{
	//check if vertex buffer succeeded
	if (&s_object.uiVBO != 0)
	{
		//bind VBO
		glBindBuffer(GL_ARRAY_BUFFER, s_object.uiVBO);

		//allocate space on graphics card
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* 4, NULL, GL_STATIC_DRAW);

		//get pointer to allocate space on graphics card
		GLvoid* vSBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		//copy to graphics card
		memcpy(vSBuffer, &s_object.vertices, sizeof(Vertex)* 4);

		//unmap and unbind
		glUnmapBuffer(GL_ARRAY_BUFFER);
		
	}

	//check if Index buffer succeeded
	if (&s_object.uiIBO != 0)
	{
		//bind IBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_object.uiIBO);
		//allocate space for index info on the graphics card
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(char), NULL, GL_STATIC_DRAW);
		//get pointer to newly allocated space on the graphics card
		GLvoid* iBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		//specify the order we'd like to draw our vertices.
		//In this case they are in sequential order
		for (int i = 0; i < 4; i++)
		{
			((char*)iBuffer)[i] = i;
		}
		//unmap and unbind
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}


	//class shader program
	glUseProgram(uiProgramTextured);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_object.spriteID);

	//ortho project onto shader
	glUniformMatrix4fv(MatrixIDTextured, 1, GL_FALSE, ortho);

	//enable vertex array state
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	//bind VBO
	glBindTexture(GL_TEXTURE_2D, s_object.spriteID);
	glBindBuffer(GL_ARRAY_BUFFER, s_object.uiVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_object.uiIBO);

	//specify where our vertex array is
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float)* 4));
	//now we have UVs, we need to send that info to the graphics card
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float)* 8));

	//draw to the screen
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, NULL);

	//unbind buffers 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

bool GLF::IsKeyPressed(KeyPressed key)
{
	return glfwGetKey(window, key);
}

bool GLF::IsMousePressed(KeyPressed key)
{
	return glfwGetMouseButton(window, key);
}

glm::vec2 GLF::MousePosition()
{
	double xPos;
	double yPos;
	
	glfwGetCursorPos(window, &xPos, &yPos);
	return glm::vec2(xPos, yPos);
}

void GLF::AddFont(const char* a_fileName)
{
	float loc[2];
	int size[2], bpp;
	bpp = 4; 
	Sprite font(a_fileName, loc, size);
	font.loadTexture(a_fileName, size[0], size[1], bpp);
}

void GLF::DrawString(const char* string, float loc[2])
{
	for (int i = 0; i < sizeof(string); i++)
	{
		
		//DrawSprite();
	}
}


void GLF::Animate(Sprite a_sprite, double locFirstSprite[2],double totalNumOfSprites[2], int numOfAnimations)
{
	Sprite S = a_sprite;
	int animCount = 0;

	//vertex one
	S.vertices[0].uv[0] = locFirstSprite[0];
	S.vertices[0].uv[1] = locFirstSprite[0];

	//vertex two
	S.vertices[1].uv[0] = locFirstSprite[0];
	S.vertices[1].uv[1] = locFirstSprite[1] + 1/ totalNumOfSprites[1];

	//vertex three
	S.vertices[1].uv[0] = locFirstSprite[0] + 1 / totalNumOfSprites[0];
	S.vertices[1].uv[1] = locFirstSprite[1] + 1 / totalNumOfSprites[1];

	//vertex three
	S.vertices[1].uv[0] = locFirstSprite[0] + 1 / totalNumOfSprites[0];
	S.vertices[1].uv[1] = locFirstSprite[1];

	while (animCount < numOfAnimations)
	{
		//vertex one
		S.vertices[0].uv[0] += 1 / totalNumOfSprites[0];

		//vertex two
		S.vertices[1].uv[0] += 1 / totalNumOfSprites[0];

		//vertex three
		S.vertices[2].uv[0] += 1 / totalNumOfSprites[1];

		//vertex four
		S.vertices[3].uv[0] += 1 / totalNumOfSprites[1];
	}
}
