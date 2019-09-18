// PhotonCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

int main()
{
	std::ifstream t("E:\\projects\\PhotonCpp\\_model.photon", std::ios_base::binary);
	t.seekg(0, std::ios::end);
	std::streamoff size = t.tellg();
	std::string buffer((unsigned int)size, ' ');
	t.seekg(0);
	t.read(&buffer[0], size);

	char * p = &buffer[0] + size;

	PhotonFileHeader photonFileHeader(buffer);
	PhotonFilePreview previewOne(photonFileHeader.getPreviewOneOffsetAddress(), buffer);
	PhotonFilePreview previewTwo(photonFileHeader.getPreviewTwoOffsetAddress(), buffer);
	
	PhotonFilePrintParameters photonFilePrintParameters;
	if (photonFileHeader.getVersion() > 1) 
	{
		PhotonFilePrintParameters dummy(photonFileHeader.getPrintParametersOffsetAddress(), buffer);
		photonFilePrintParameters = std::move(dummy);
	}

	int margin = 0;
	
	std::vector<PhotonFileLayer> layers = PhotonFileLayer::readLayers(photonFileHeader, buffer, margin);

	PhotonFileLayer::calculateLayers(photonFileHeader,layers, 0);




	// writing is here:

	std::ofstream ofs("E:\\projects\\PhotonCpp\\_out.photon", std::ios_base::binary);

	int antiAliasLevel = 1;
	if (photonFileHeader.getVersion() > 1) {
		antiAliasLevel = photonFileHeader.getAntiAliasingLevel();
	}

	int headerPos = 0;
	int previewOnePos = headerPos + photonFileHeader.getByteSize();
	int previewTwoPos = previewOnePos + previewOne.getByteSize();
	int layerDefinitionPos = previewTwoPos + previewTwo.getByteSize();

	int parametersPos = 0;
	if (photonFileHeader.getVersion() > 1) {
		parametersPos = layerDefinitionPos;
		layerDefinitionPos = parametersPos + photonFilePrintParameters.getByteSize();
	}

	int dataPosition = layerDefinitionPos + (PhotonFileLayer::getByteSize() * photonFileHeader.getNumberOfLayers() * antiAliasLevel);


	PhotonOutputStream os(ofs);

	photonFileHeader.save(os, previewOnePos, previewTwoPos, layerDefinitionPos, parametersPos);
	previewOne.save(os, previewOnePos);
	previewTwo.save(os, previewTwoPos);

	if (photonFileHeader.getVersion() > 1) {
		photonFilePrintParameters.save(os);
	}

	// Optimize order for speed read on photon
	for (int i = 0; i < photonFileHeader.getNumberOfLayers(); i++) {
		PhotonFileLayer layer = layers.at(i);
		dataPosition = layer.savePos(dataPosition);
		if (antiAliasLevel > 1) {
			for (int a = 0; a < (antiAliasLevel - 1); a++) {
				dataPosition = layer.getAntiAlias(a).savePos(dataPosition);
			}
		}
	}

	// Order for backward compatibility with photon/cbddlp version 1
	for (int i = 0; i < photonFileHeader.getNumberOfLayers(); i++) {
		layers.at(i).save(os);
	}

	if (antiAliasLevel > 1) {
		for (int a = 0; a < (antiAliasLevel - 1); a++) {
			for (int i = 0; i < photonFileHeader.getNumberOfLayers(); i++) {
				layers.at(i).getAntiAlias(a).save(os);
			}
		}
	}

	// Optimize order for speed read on photon
	for (int i = 0; i < photonFileHeader.getNumberOfLayers(); i++) {
		PhotonFileLayer layer = layers.at(i);
		layer.saveData(os);
		if (antiAliasLevel > 1) {
			for (int a = 0; a < (antiAliasLevel - 1); a++) {
				layer.getAntiAlias(a).saveData(os);
			}
		}
	}

    std::cout << "Hello World!\n"; 

	return 0;
}