/****************************************************************************************

   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.

   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.

****************************************************************************************/

#include "ImportExport.h"
#include <set>

// declare global
FbxManager*   gSdkManager = NULL;

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(gSdkManager->GetIOSettings()))
#endif


// a UI file provide a function to print messages
extern void UI_Printf(const char* msg, ...);

// to read and write a file using the FBXSDK readers/writers
//
// const char *ImportFileName : the full path of the file to be read
// const char* ExportFileName : the full path of the file to be written
// int pWriteFileFormat       : the specific file format number
//                                  for the writer

void ImportExport(
                  const char *ImportFileName,
	              const char* ImportFileName2,
                  const char* ExportFileName,
                  int pWriteFileFormat
                  )
{
	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager,"");
    FbxScene* lScene2 = FbxScene::Create(gSdkManager, "");

    UI_Printf("------- Import started ---------------------------");

    // Load the scene.
    bool r = LoadScene(gSdkManager, lScene, ImportFileName);
    if(r)
        UI_Printf("------- Import succeeded -------------------------");
    else
    {
        UI_Printf("------- Import failed ----------------------------");

        // Destroy the scene
		lScene->Destroy();
        return;
    }

	// Load the scene.
    r = LoadScene(gSdkManager, lScene2, ImportFileName2);
	if (r)
		UI_Printf("------- Import succeeded -------------------------");
	else
	{
		UI_Printf("------- Import failed ----------------------------");

		// Destroy the scene
		lScene2->Destroy();
		return;
	}

    UI_Printf("\r\n"); // add a blank line

    // merge normal form outline mesh to lighting mesh
    ProcessNode(lScene->GetRootNode(), lScene2->GetRootNode());

    UI_Printf("------- Export started ---------------------------");

    // Save the scene.
    r = SaveScene(gSdkManager, 
        lScene,               // to export this scene...
        ExportFileName,       // to this path/filename...
        pWriteFileFormat,     // using this file format.
        false);               // Don't embed media files, if any.

    if(r) UI_Printf("------- Export succeeded -------------------------");
    else  UI_Printf("------- Export failed ----------------------------");

	// destroy the scene
	lScene->Destroy();
}

// Creates an instance of the SDK manager.
void InitializeSdkManager()
{
    // Create the FBX SDK memory manager object.
    // The SDK Manager allocates and frees memory
    // for almost all the classes in the SDK.
    gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings * ios = FbxIOSettings::Create(gSdkManager, IOSROOT );
	gSdkManager->SetIOSettings(ios);

}

// Destroys an instance of the SDK manager
void DestroySdkObjects(
                       FbxManager* pSdkManager,
					   bool pExitStatus
                       )
{
    // Delete the FBX SDK manager.
    // All the objects that
    // (1) have been allocated by the memory manager, AND that
    // (2) have not been explicitly destroyed
    // will be automatically destroyed.
    if( pSdkManager ) pSdkManager->Destroy();
	if( pExitStatus ) FBXSDK_printf("Program Success!\n");
}

// Creates an importer object, and uses it to
// import a file into a scene.
bool LoadScene(
               FbxManager* pSdkManager,  // Use this memory manager...
               FbxScene* pScene,            // to import into this scene
               const char* pFilename         // the data from this file.
               )
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor,  lSDKMinor,  lSDKRevision;
    int i, lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the version number of the FBX files generated by the
    // version of FBX SDK that you are using.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pSdkManager,"");
    
    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pSdkManager->GetIOSettings() );

    // Get the version number of the FBX file format.
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if( !lImportStatus )  // Problem with the file to be imported
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
        UI_Printf("Call to FbxImporter::Initialize() failed.");
        UI_Printf("Error returned: %s", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
                lSDKMajor, lSDKMinor, lSDKRevision);
            UI_Printf("FBX version number for file %s is %d.%d.%d",
                pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
        lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
        UI_Printf("FBX version number for file %s is %d.%d.%d",
            pFilename, lFileMajor, lFileMinor, lFileRevision);

        // In FBX, a scene can have one or more "animation stack". An animation stack is a
        // container for animation data.
        // You can access a file's animation stack information without
        // the overhead of loading the entire file into the scene.

        UI_Printf("Animation Stack Information");

        lAnimStackCount = lImporter->GetAnimStackCount();

        UI_Printf("    Number of animation stacks: %d", lAnimStackCount);
        UI_Printf("    Active animation stack: \"%s\"",
            lImporter->GetActiveAnimStackName().Buffer());

        for(i = 0; i < lAnimStackCount; i++)
        {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            UI_Printf("    Animation Stack %d", i);
            UI_Printf("         Name: \"%s\"", lTakeInfo->mName.Buffer());
            UI_Printf("         Description: \"%s\"",
                lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should
            // be imported under a different name.
            UI_Printf("         Import Name: \"%s\"", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false
            // if the animation stack should be not be imported.
            UI_Printf("         Import State: %s", lTakeInfo->mSelect ? "true" : "false");
        }

        // Import options determine what kind of data is to be imported.
        // The default is true, but here we set the options explictly.

        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL,        true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE,         true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK,            true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE,           true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO,            true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION,       true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);


    }

	// new 
    IOS_REF.SetBoolProp(IMP_FBX_NORMAL, true);
	IOS_REF.SetBoolProp(IMP_FBX_BINORMAL, true);
	IOS_REF.SetBoolProp(IMP_FBX_TANGENT, true);
	IOS_REF.SetBoolProp(IMP_FBX_VERTEXCOLOR, true);
	IOS_REF.SetBoolProp(IMP_FBX_SMOOTHING, true);
	IOS_REF.SetBoolProp(IMP_SMOOTHING_GROUPS, true);

    // Import the scene.
    lStatus = lImporter->Import(pScene);

    if(lStatus == false &&     // The import file may have a password
        lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
    {
        UI_Printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END
        FbxString lString(lPassword);
        
        IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
        IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);


        lStatus = lImporter->Import(pScene);

        if(lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
        {
            UI_Printf("Incorrect password: file not imported.");
        }
    }

    // Destroy the importer
    lImporter->Destroy();

    return lStatus;
}

// Exports a scene to a file
bool SaveScene(
               FbxManager* pSdkManager,
               FbxScene* pScene,
               const char* pFilename,
               int pFileFormat,
               bool pEmbedMedia
               )
{
    int lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create(pSdkManager, "");

    if( pFileFormat < 0 ||
        pFileFormat >=
        pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount() )
    {
        // Write in fall back format if pEmbedMedia is true
        pFileFormat =
            pSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
     
        if (!pEmbedMedia)
        {
            //Try to export in ASCII if possible
            int lFormatIndex, lFormatCount =
                pSdkManager->GetIOPluginRegistry()->
                GetWriterFormatCount();

            for (lFormatIndex=0; lFormatIndex<lFormatCount; lFormatIndex++)
            {
                if (pSdkManager->GetIOPluginRegistry()->
                    WriterIsFBX(lFormatIndex))
                {
                    FbxString lDesc = pSdkManager->GetIOPluginRegistry()->
                        GetWriterFormatDescription(lFormatIndex);
                    if (lDesc.Find("ascii")>=0)
                    {
                        pFileFormat = lFormatIndex;
                        break;
                    }
                }
            }
        }
    }

    // Initialize the exporter by providing a filename.
    if(lExporter->Initialize(pFilename, pFileFormat, pSdkManager->GetIOSettings()) == false)
    {
        UI_Printf("Call to FbxExporter::Initialize() failed.");
        UI_Printf("Error returned: %s", lExporter->GetStatus().GetErrorString());
        return false;
    }

    FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
    UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
        lMajor, lMinor, lRevision);

    if (pSdkManager->GetIOPluginRegistry()->WriterIsFBX(pFileFormat))
    {

        // Export options determine what kind of data is to be imported.
        // The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
        // is true, but here we set the options explicitly.
        IOS_REF.SetBoolProp(EXP_FBX_MATERIAL,        true);
        IOS_REF.SetBoolProp(EXP_FBX_TEXTURE,         true);
        IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED,        pEmbedMedia);
        IOS_REF.SetBoolProp(EXP_FBX_SHAPE,           true);
        IOS_REF.SetBoolProp(EXP_FBX_GOBO,            true);
        IOS_REF.SetBoolProp(EXP_FBX_ANIMATION,       true);
        IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
        
        // new
		IOS_REF.SetBoolProp(EXP_SMOOTHING_GROUPS, true);
    }


    // Export the scene.
    lStatus = lExporter->Export(pScene);

    // Destroy the exporter.
    lExporter->Destroy();

    return lStatus;
}

void ProcessNode(FbxNode* pNode,FbxNode* pNode2)
{
    if (pNode->GetNodeAttribute() && pNode2->GetNodeAttribute())
    {
		if (pNode->GetNodeAttribute()->GetAttributeType() != pNode2->GetNodeAttribute()->GetAttributeType())
		{
            UI_Printf("------- ERROR! Input Mesh don't match! ---------------------------");
            return;
		}
		else
		{
            switch (pNode->GetNodeAttribute()->GetAttributeType())
            {
            case FbxNodeAttribute::EType::eMesh:
                ProcessMesh(pNode, pNode2);
                break;
            default:
                break;
            }
		}
    }
    
    int ChildCount = pNode->GetChildCount();
    int ChildCount2 = pNode2->GetChildCount();
    if (ChildCount != ChildCount2)
    {
		UI_Printf("------- ERROR! Input Mesh don't match! ---------------------------");
		return;
    }
    else
    {
        for (int i = 0; i < ChildCount; ++i)
        {
            ProcessNode(pNode->GetChild(i), pNode2->GetChild(i));
        }
    }
}

void ProcessMesh(FbxNode* pNode, FbxNode* pNode2)
{
    // get mesh
    FbxMesh* pMesh = pNode->GetMesh();
    FbxMesh* pMesh2 = pNode2->GetMesh();

    if (pMesh == nullptr || pMesh2 == nullptr)
    {
        UI_Printf("------- ERROR! Input Mesh don't match! ---------------------------");
        return;
    }

	FbxGeometryElementNormal* lNormalElementSrc = pMesh2->GetElementNormal(0);
	FbxGeometryElementNormal* lNormalElementDst = pMesh->GetElementNormal(0);
    FbxGeometryElementTangent* lTangentElement = pMesh->GetElementTangent(0);
    FbxGeometryElementBinormal* lBinormalElement = pMesh->GetElementBinormal(0);

    if (lTangentElement == nullptr)
    {
        lTangentElement = pMesh->CreateElementTangent();
    }
    if (lBinormalElement == nullptr)
    {
        lBinormalElement = pMesh->CreateElementBinormal();
    }

    //lTangentElement->Clear();
    //lBinormalElement->Clear();
	int Size = lNormalElementSrc->GetDirectArray().GetCount();
	lTangentElement->GetDirectArray().SetCount(Size);
    lBinormalElement->GetDirectArray().SetCount(Size);
	
    lTangentElement->SetMappingMode(lNormalElementDst->GetMappingMode());
	lBinormalElement->SetMappingMode(lNormalElementDst->GetMappingMode());
    lTangentElement->SetReferenceMode(lNormalElementDst->GetReferenceMode());
	lBinormalElement->SetReferenceMode(lNormalElementDst->GetReferenceMode());

	if (lNormalElementSrc && lTangentElement && lBinormalElement)
	{
		if (lNormalElementSrc->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{
			//Let's get normals of each vertex, since the mapping mode of normal element is by control point
			for (int lVertexIndex = 0; lVertexIndex < pMesh->GetControlPointsCount(); lVertexIndex++)
			{
				int lNormalIndex = 0;
				//reference mode is direct, the normal index is same as vertex index.
				//get normals by the index of control vertex
				if (lNormalElementSrc->GetReferenceMode() == FbxGeometryElement::eDirect)
					lNormalIndex = lVertexIndex;

				//reference mode is index-to-direct, get normals by the index-to-direct
				if (lNormalElementSrc->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					lNormalIndex = lNormalElementSrc->GetIndexArray().GetAt(lVertexIndex);

				//Got normals of each vertex.
				FbxVector4 lNormal = lNormalElementDst->GetDirectArray().GetAt(lNormalIndex);
				//Got normals of each polygon-vertex.
				FbxVector4 lTangent = lNormalElementSrc->GetDirectArray().GetAt(lNormalIndex);
				lTangent.Normalize();
				FbxVector4 lBitangent = lNormal.CrossProduct(lTangent);
                
                int lTangentIndex = 0;
                if (lTangentElement->GetReferenceMode() == FbxLayerElement::eDirect)
                    lTangentIndex = lVertexIndex;
                if (lTangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                    lTangentIndex = lTangentElement->GetIndexArray().GetAt(lVertexIndex);

				lTangentElement->GetDirectArray().SetAt(lTangentIndex, lTangent);
				lBinormalElement->GetDirectArray().SetAt(lTangentIndex, lBitangent);

			}//end for lVertexIndex
		}//end eByControlPoint

		else if (lNormalElementSrc->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int lIndexByPolygonVertex = 0;
			//Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
			for (int lPolygonIndex = 0; lPolygonIndex < pMesh->GetPolygonCount(); lPolygonIndex++)
			{
				//get polygon size, you know how many vertices in current polygon.
				int lPolygonSize = pMesh->GetPolygonSize(lPolygonIndex);

				//retrieve each vertex of current polygon.
				for (int i = 0; i < lPolygonSize; i++)
				{
					int lNormalIndex = 0;
					//reference mode is direct, the normal index is same as lIndexByPolygonVertex.
					if (lNormalElementSrc->GetReferenceMode() == FbxGeometryElement::eDirect)
						lNormalIndex = lIndexByPolygonVertex;

					//reference mode is index-to-direct, get normals by the index-to-direct
					if (lNormalElementSrc->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						lNormalIndex = lNormalElementSrc->GetIndexArray().GetAt(lIndexByPolygonVertex);

                    FbxVector4 lNormal = lNormalElementDst->GetDirectArray().GetAt(lNormalIndex);

					//Got normals of each polygon-vertex.
					FbxVector4 lTangent = lNormalElementSrc->GetDirectArray().GetAt(lNormalIndex);
                    lTangent.Normalize();

                    FbxVector4 lBitangent = lNormal.CrossProduct(lTangent);

					int lTangentIndex = 0;
					if (lTangentElement->GetReferenceMode() == FbxLayerElement::eDirect)
						lTangentIndex = lIndexByPolygonVertex;
					if (lTangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						lTangentIndex = lTangentElement->GetIndexArray().GetAt(lIndexByPolygonVertex);

                    lTangentElement->GetDirectArray().SetAt(lTangentIndex, lTangent);
                    lBinormalElement->GetDirectArray().SetAt(lTangentIndex, lBitangent);

                    lIndexByPolygonVertex++;

				}//end for i //lPolygonSize
			}//end for lPolygonIndex //PolygonCount
		}//end eByPolygonVertex

	}//end if lNormalElementSrc

}

// Get the filters for the <Open file> dialog
// (description + file extention)
const char *GetReaderOFNFilters()
{
    int nbReaders =
		gSdkManager->GetIOPluginRegistry()->GetReaderFormatCount();

    FbxString s;
    int i = 0;

    for(i=0; i < nbReaders; i++)
    {
        s += gSdkManager->GetIOPluginRegistry()->
            GetReaderFormatDescription(i);
        s += "|*.";
        s += gSdkManager->GetIOPluginRegistry()->
            GetReaderFormatExtension(i);
        s += "|";
    }

    // replace | by \0
    int nbChar   = int(strlen(s.Buffer())) + 1;
    char *filter = new char[ nbChar ];
    memset(filter, 0, nbChar);

    FBXSDK_strcpy(filter, nbChar, s.Buffer());

    for(i=0; i < int(strlen(s.Buffer())); i++)
    {
        if(filter[i] == '|')
        {
            filter[i] = 0;
        }
    }

    // the caller must delete this allocated memory
    return filter;
}

// Get the filters for the <Save file> dialog
// (description + file extention)
const char *GetWriterSFNFilters()
{
    int nbWriters =
        gSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();

    FbxString s;
    int i=0;

    for(i=0; i < nbWriters; i++)
    {
        s += gSdkManager->GetIOPluginRegistry()->
            GetWriterFormatDescription(i);
        s += "|*.";
        s += gSdkManager->GetIOPluginRegistry()->
            GetWriterFormatExtension(i);
        s += "|";
    }

    // replace | by \0
    int nbChar   = int(strlen(s.Buffer())) + 1;
    char *filter = new char[ nbChar ];
    memset(filter, 0, nbChar);

    FBXSDK_strcpy(filter, nbChar, s.Buffer());

    for(i=0; i < int(strlen(s.Buffer())); i++)
    {
        if(filter[i] == '|')
        {
            filter[i] = 0;
        }
    }

    // the caller must delete this allocated memory
    return filter;
}

// to get a file extention for a WriteFileFormat
const char *GetFileFormatExt(
                             const int pWriteFileFormat
                             )
{
    char *buf = new char[10];
    memset(buf, 0, 10);

    // add a starting point .
    buf[0] = '.';
    const char * ext = gSdkManager->GetIOPluginRegistry()->
        GetWriterFormatExtension(pWriteFileFormat);
    FBXSDK_strcat(buf, 10, ext);

    // the caller must delete this allocated memory
    return buf;
}