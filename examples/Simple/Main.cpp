//#include "bmdl.h"

//BmDataNode dataBlock;

#include <iostream>

int main()
{
	uint32_t BmFileID = 'B' | ('M' << 8) | ('D' << 16) | ('L' << 24);
	std::cout << BmFileID<< "\n";
	system("pause");
	//bmdl::LoadModel<BmVert, uint16_t>("sponza.bmf");
	/*BmModel* MyModel = bmdl::LoadModel("sponza.bmf");

	for (uint32_t m = 0; m < 256; m++)
	{
		BmDataNode* matNode = dataBlock.AddNode("material");
		matNode->AddAttribute("mat_name", "BoxMaterial");
		matNode->AddAttribute("diffuse", "Box/boxDiff.tga");
		matNode->AddAttribute("specular", "Box/boxSpec.tga");
		matNode->AddAttribute("spec_power", 1.3f);
		matNode->AddAttribute("color", BmColor32(255, 0, 0));
	}

	system("pause");
	BmDataBlock::SaveBlock(&dataBlock, "E:/dev/output/test.bmd");

	system("pause");*/

	// Load static model
	/*BmMesh* MyMesh = bmdl::LoadStaticMesh("sponza.bms");
	
	// Access static model materials/values
	BmMaterialData* MyMaterial = MyModel->GetMaterial(0);
	MyMaterial->GetValue<float>("specular");
	MyMaterial->GetStringValue("diffuse");
	
	// Accessing extensions data
	BmDataElement* ExtensionData = MyModel->GetExtensionData("Attachments");*/
	
	return 0;
}