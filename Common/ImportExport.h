/****************************************************************************************

   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.

   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.

****************************************************************************************/

// use the fbxsdk.h
#include <fbxsdk.h>

void ImportExport(
                    const char *ImportFileName, 
                    const char* ImportFileName2,
                    const char* ExportFileName, 
                    int pWriteFileFormat
                 );


void InitializeSdkManager();

void DestroySdkObjects(
                            FbxManager* pSdkManager,
							bool pExitStatus
                      );

const char *GetReaderOFNFilters();

const char *GetWriterSFNFilters();

const char *GetFileFormatExt(
                              const int pWriteFileFormat 
                            );

bool LoadScene(
                FbxManager* pSdkManager, 
                FbxScene* pScene, 
                const char* pFilename
              );

bool SaveScene(
                FbxManager* pSdkManager, 
                FbxScene* pScene, 
                const char* pFilename, 
                int pFileFormat, 
                bool pEmbedMedia
              );

void ProcessNode(FbxNode* pNode, FbxNode* pNode2);
void ProcessMesh(FbxNode* pNode, FbxNode* pNode2);

void ReadNormal(FbxMesh* pMesh, int ctrlPointIndex, int vertexCounter, FbxVector4& OutNormal);
void ReadTangent(FbxMesh* pMesh, int ctrlPointIndex, int vertexCounter, FbxVector4& OutTangent);

void WriteNormal(FbxMesh* pMesh, int ctrlPointIndex, int vertexCounter, FbxVector4& Normal);
void WriteTangent(FbxMesh* pMesh, int ctrlPointIndex, int vertexCounter, FbxVector4& Tangent);
void WriteBitTangent(FbxMesh* pMesh, int ctrlPointIndex, int vertexCounter, FbxVector4& BiTangent);


