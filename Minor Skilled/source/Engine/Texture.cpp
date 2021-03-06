#include "Texture.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../../dependencies/stb_image/stb_image.h"

#include "../Utility/Filepath.h"

Texture::Texture(GLenum target, GLenum internalFormat, unsigned int width, unsigned int height, GLenum format, GLenum type, GLenum minFilter, GLenum magFilter, GLenum wrap, const void* pixels, bool genMipmaps):
 _target(target){

	glGenTextures(1, &_id);

	bind();

	init(internalFormat, width, height, format, type, pixels);
	filter(minFilter, magFilter, wrap);

	if(genMipmaps) generateMipmaps();
}

Texture::Texture(GLenum target): _target(target) {
	glGenTextures(1, &_id);
}

Texture::~Texture() {
	glDeleteTextures(1, &_id);
}

unsigned int& Texture::getID() {
	return _id;
}

void Texture::bind() {
	glBindTexture(_target, _id); //bind texture to target
}

void Texture::generateMipmaps() {
	glGenerateMipmap(_target);
}

void Texture::init(GLenum internalFormat, unsigned int width, unsigned int height, GLenum format, GLenum type, const void* pixels) {
	glTexImage2D(_target, 0, internalFormat, width, height, 0, format, type, pixels);
}

void Texture::initTarget(GLenum target, GLenum internalFormat, unsigned int width, unsigned int height, GLenum format, GLenum type, const void * pixels) {
	glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, pixels);
}

void Texture::filter(GLenum minFilter, GLenum magFilter, GLenum wrap) {
	glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, magFilter);

	if(wrap != GL_NONE) {
		glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrap);

		if(_target == GL_TEXTURE_CUBE_MAP) glTexParameteri(_target, GL_TEXTURE_WRAP_R, wrap);
	}
}

Texture * Texture::LoadTexture(std::string path, TextureFilter filter, bool sRGB) { 
	//sRGB textures are essentially gamma corrected already and usually the colorspace they are created in
	//when setting the sRGB parameter to true, OpenGL transforms  the texture from gamma corrected/sRGB color space back to linear color space so that they can/have to be gamma corrected in the shaders
	//diffuse and color textures are almost always in sRGB space - specular map, normals maps, etc. are almost always in linear space

	stbi_set_flip_vertically_on_load(false);

	//create opengl texture object
	Texture* texture = new Texture(GL_TEXTURE_2D);

	texture->filepath = path;

	//load texture from file
	int width, height, nrComponents;
	unsigned char* textureData = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

	if(textureData) {
		//identify format
		GLenum internalFormat;
		GLenum dataFormat;

		if(nrComponents == 1) {
			internalFormat = dataFormat = GL_RED;
		} else if(nrComponents == 3) {
			internalFormat = sRGB ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		} else if(nrComponents == 4) {
			internalFormat = sRGB ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}

		//load texture into opengl
		texture->bind();
		texture->init(internalFormat, width, height, dataFormat, GL_UNSIGNED_BYTE, textureData);
		texture->generateMipmaps();

		//set texture filter options
		switch(filter) {
			case TextureFilter::Repeat:
				texture->filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
				break;

			case TextureFilter::ClampToEdge:
				texture->filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
				break;
		}

		stbi_image_free(textureData); //free memory

	} else {
		std::cout << "Texture failed to load at path: " + path << std::endl;
		delete texture;
		stbi_image_free(textureData); //free memory
		return nullptr;
	}

	return texture;
}

Texture * Texture::LoadCubemap(std::vector<std::string>& faces, bool sRGB) {
	stbi_set_flip_vertically_on_load(false);

	Texture* texture = new Texture(GL_TEXTURE_CUBE_MAP);
	texture->bind();

	texture->filepath = Filepath::SkyboxPath + faces[0];

	GLenum internalFormat = sRGB ? GL_SRGB : GL_RGB;
	int width, height, nrChannels;
	std::string filename;

	for(unsigned int i = 0; i < faces.size(); i++) {
		filename = Filepath::SkyboxPath + faces[i];

		unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
		if(data) {
			texture->initTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, internalFormat, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " + faces[i] << std::endl;
			delete texture;
			stbi_image_free(data);
			return nullptr;
		}
	}

	texture->filter(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

	return texture;
}

Texture* Texture::LoadHDR(std::string path) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);

	if(data) {
		Texture* texture = new Texture(GL_TEXTURE_2D);
		texture->bind();
		texture->init(GL_RGB16F, width, height, GL_RGB, GL_FLOAT, data);

		texture->filter(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

		texture->filepath = path;

		stbi_image_free(data);

		return texture;
	} else {
		std::cout << "Failed to load HDR image." << std::endl;
		stbi_image_free(data);
		return nullptr;
	}
}

void Texture::Unbind(GLenum target) {
	//static helper method to unbind textures from targets since textures can potentially be nullptr
	glBindTexture(target, 0);
}

void Texture::SetActiveUnit(unsigned int unit) {
	glActiveTexture(GL_TEXTURE0 + unit);
}
