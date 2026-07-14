#pragma once
#include "DirectXMath.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>

typedef unsigned char byte;

namespace Utils
{
	static double GetCurrentTimeMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch())
			.count();
	}

	template<typename StructType, typename T>
	static std::vector<T> ExtractFromStructList(const std::vector<StructType> structList, T StructType::* field)
	{
		std::vector<T> fieldList{};
		fieldList.reserve(structList.size());

		for (const StructType& structElement : structList)
		{
			fieldList.push_back(structElement.*field);
		}
		return fieldList;
	}

	static void LoadBinaryData(const std::string& binarySrcPath, byte*& dst, size_t& binSize)
	{
		std::ifstream binFile{ binarySrcPath, std::ios::binary | std::ios::ate };
		if (!binFile.is_open())
		{
			std::cout << "Did not find binary at " << binarySrcPath << "\n";
			return;
		}

		if (binSize == 0)
		{
			binSize = binFile.tellg();
		}

		if (dst == nullptr)
		{
			dst = new byte[binSize];
		}

		binFile.seekg(0, std::ios::beg);
		binFile.read((char*)dst, binSize);
	}

	static std::vector<float> xmMatrixToVector(const DirectX::XMMATRIX& matrix)
	{
		DirectX::XMFLOAT4X4 storedMatrix;
		DirectX::XMStoreFloat4x4(&storedMatrix, matrix);

		return {
			storedMatrix._11, storedMatrix._12, storedMatrix._13, storedMatrix._14,
			storedMatrix._21, storedMatrix._22, storedMatrix._23, storedMatrix._24,
			storedMatrix._31, storedMatrix._32, storedMatrix._33, storedMatrix._34,
			storedMatrix._41, storedMatrix._42, storedMatrix._43, storedMatrix._44
		};
	}

	static void printMatrix(const DirectX::XMMATRIX& m, const std::string& matrixName = "")
	{
		DirectX::XMFLOAT4X4 mat;
		XMStoreFloat4x4(&mat, m);
		std::cout << matrixName << "\n";
		std::cout << mat._11 << " " << mat._12 << " " << mat._13 << " " << mat._14 << "\n";
		std::cout << mat._21 << " " << mat._22 << " " << mat._23 << " " << mat._24 << "\n";
		std::cout << mat._31 << " " << mat._32 << " " << mat._33 << " " << mat._34 << "\n";
		std::cout << mat._41 << " " << mat._42 << " " << mat._43 << " " << mat._44 << "\n";
	}

	inline std::string HrToAString(HRESULT hr)
	{
		char* buffer = nullptr;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&buffer,
			0,
			nullptr
		);

		std::string message = buffer ? buffer : "(unknown error)";
		LocalFree(buffer);
		return message;
	}
}
