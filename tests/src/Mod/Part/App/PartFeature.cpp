// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartCommon.h"
#include <src/App/InitApplication.h>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include "PartTestHelpers.h"

using namespace Part;
using namespace PartTestHelpers;

class FeaturePartTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    // TODO:  See below for where this comparable code in LS3 doesn't happen:
    //    >>> App.getDocument('Unnamed').addObject("Part::MultiCommon","Common")
    //    >>> App.getDocument('Unnamed').getObject('Common').Shapes =
    //    [FreeCAD.getDocument('Unnamed').getObject('Box001'),FreeCAD.getDocument('Unnamed').getObject('Box002'),]
    //    >>> App.getDocument('Unnamed').getObject('Box001').Visibility = False
    //    >>> App.getDocument('Unnamed').getObject('Box002').Visibility = False
    //    >>> App.ActiveDocument.recompute()
    //    >>> o2 = App.ActiveDocument.Objects[-1]
    //    >>> o2.Shape.ElementMap
    //    {'Edge10;:M;CMN;:H19c:7,E': 'Edge7', 'Edge11;:M;CMN;:H19b:7,E': 'Edge8',
    //    'Edge12;:M;CMN;:H19b:7,E': 'Edge5', 'Edge1;:M;CMN;:H19c:7,E': 'Edge4',
    //    'Edge2;:M#9;CMN;:H19b:9,E': 'Edge1', 'Edge3;:M;CMN;:H19b:7,E': 'Edge2',
    //    'Edge4;:M#a;CMN;:H19b:9,E': 'Edge3', 'Edge5;:M;CMN;:H19c:7,E': 'Edge12',
    //    'Edge6;:M#b;CMN;:H19b:9,E': 'Edge6', 'Edge7;:M;CMN;:H19b:7,E': 'Edge9',
    //    'Edge8;:M#c;CMN;:H19b:9,E': 'Edge10', 'Edge9;:M;CMN;:H19c:7,E': 'Edge11',
    //    'Face1;:M#d;CMN;:H19b:9,F': 'Face1', 'Face2;:M#10;CMN;:H19b:a,F': 'Face6',
    //    'Face3;:M;CMN;:H19c:7,F': 'Face5', 'Face4;:M;CMN;:H19b:7,F': 'Face3',
    //    'Face5;:M#f;CMN;:H19b:9,F': 'Face4', 'Face6;:M#e;CMN;:H19b:9,F': 'Face2',
    //    'Vertex1;:M;CMN;:H19c:7,V': 'Vertex1', 'Vertex2;:M;CMN;:H19c:7,V': 'Vertex4',
    //    'Vertex3;:M;CMN;:H19b:7,V': 'Vertex2', 'Vertex4;:M;CMN;:H19b:7,V': 'Vertex3',
    //    'Vertex5;:M;CMN;:H19c:7,V': 'Vertex6', 'Vertex6;:M;CMN;:H19c:7,V': 'Vertex8',
    //    'Vertex7;:M;CMN;:H19b:7,V': 'Vertex5', 'Vertex8;:M;CMN;:H19b:7,V': 'Vertex7'}


    void SetUp() override
    {
        createTestDoc();
        _common = dynamic_cast<MultiCommon*>(_doc->addObject("Part::MultiCommon"));
    }

    void TearDown() override
    {}

    MultiCommon* _common = nullptr;
};

TEST_F(FeaturePartTest, testGetElementName)
{
    // Arrange
    _common->Shapes.setValue({_boxes[0], _boxes[1]});
    // Act
    _common->execute();
    const TopoShape& ts = _common->Shape.getShape();

    auto namePair = _common->getElementName("test");
    auto namePairExport = _common->getElementName("test", App::GeoFeature::Export);
    auto namePairSelf = _common->getElementName(nullptr);
    // Assert
    EXPECT_STREQ(namePair.first.c_str(), "");
    EXPECT_STREQ(namePair.second.c_str(), "test");
    EXPECT_STREQ(namePairExport.first.c_str(), "");
    EXPECT_STREQ(namePairExport.second.c_str(), "test");
    EXPECT_STREQ(namePairSelf.first.c_str(), "");
    EXPECT_STREQ(namePairSelf.second.c_str(), "");
#ifndef FC_USE_TNP_FIX
    EXPECT_EQ(ts.getElementMap().size(), 0);
#else
    // TODO: This next call should not be necessary, and is evidence that we are not setting the
    // elementMaps up correctly.  Same call from the python layer in LS3 does create the elementMap.
    _common->Shape.getShape().mapSubElement(_common->Shape.getShape().getSubTopoShapes());
    EXPECT_EQ(ts.getElementMap().size(), 26);  // TODO: Value and code TBD
#endif
    // TBD
}

TEST_F(FeaturePartTest, create)
{
    // Arrange

    // A shape that will be passed to the various calls of Feature::create
    auto shape {TopoShape(BRepBuilderAPI_MakeVertex(gp_Pnt(1.0, 1.0, 1.0)).Vertex(), 1)};

    auto otherDocName {App::GetApplication().getUniqueDocumentName("otherDoc")};
    // Another document where it will be created a shape
    auto otherDoc {App::GetApplication().newDocument(otherDocName.c_str(), "otherDocUser")};

    // _doc is populated by PartTestHelperClass::createTestDoc. Making it an empty document
    _doc->clearDocument();

    // Setting the active document back to _doc otherwise the first 3 calls to Feature::create will
    // act on otherDoc
    App::GetApplication().setActiveDocument(_doc);

    // Act

    // A feature with an empty TopoShape
    auto featureNoShape {Feature::create(TopoShape())};

    // A feature with a TopoShape
    auto featureNoName {Feature::create(shape)};

    // A feature with a TopoShape and a name
    auto featureNoDoc {Feature::create(shape, "Vertex")};

    // A feature with a TopoShape and a name in the document otherDoc
    auto feature {Feature::create(shape, "Vertex", otherDoc)};

    // Assert

    // Check that the shapes have been added. Only featureNoShape should return an empty shape, the
    // others should have it as TopoShape shape is passed as argument
    EXPECT_TRUE(featureNoShape->Shape.getValue().IsNull());
    EXPECT_FALSE(featureNoName->Shape.getValue().IsNull());
    EXPECT_FALSE(featureNoDoc->Shape.getValue().IsNull());
    EXPECT_FALSE(feature->Shape.getValue().IsNull());

    // Check the features names

    // Without a name the feature's name will be set to "Shape"
    EXPECT_STREQ(_doc->getObjectName(featureNoShape), "Shape");

    // In _doc there's already a shape with name "Shape" and, as there can't be duplicated names in
    // the same document, the other feature will get an unique name that will still contain "Shape"
    EXPECT_STREQ(_doc->getObjectName(featureNoName), "Shape001");

    // There aren't other features with name "Vertex" in _doc, therefor that name will be assigned
    // without modifications
    EXPECT_STREQ(_doc->getObjectName(featureNoDoc), "Vertex");

    // The feature is created in otherDoc, which doesn't have other features and thertherefor the
    // feature's name will be assigned without modifications
    EXPECT_STREQ(otherDoc->getObjectName(feature), "Vertex");

    // Check that the features have been created in the correct document

    // The first 3 calls to Feature::create acts on _doc, which is empty, and therefor the number of
    // features in that document is the same of the features created with Feature::create
    EXPECT_EQ(_doc->getObjects().size(), 3);

    // The last call to Feature::create acts on otherDoc, which is empty, and therefor that document
    // will have only 1 feature
    EXPECT_EQ(otherDoc->getObjects().size(), 1);
}

TEST_F(FeaturePartTest, testGetTopoShape)
{
    // Arrange
    _common->Shapes.setValue({_boxes[0], _boxes[1]});
    //    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    //    EXPECT_EQ(cube1.getElementMapSize(),54);
    // Act
    _common->execute();
    // TODO: This next call should not be necessary, and is evidence that we are not setting the
    // elementMaps up correctly.  Same call from the python layer in LS3 does create the elementMap.
    _common->Shape.getShape().mapSubElement(_common->Shape.getShape().getSubTopoShapes());
    const auto& topoShape1 = _common->getTopoShape(_common);
    const auto& topoShape2 = _common->getTopoShape(_common, "Part__Box");
    // Assert
    EXPECT_STREQ(_boxes[0]->Shape.getShape().shapeName().c_str(), "Solid");
    EXPECT_EQ(topoShape1.getSubTopoShapes().size(), 1);
    EXPECT_EQ(topoShape1.getElementMapSize(), 0);
    EXPECT_EQ(topoShape2.getSubTopoShapes().size(), 1);
    EXPECT_EQ(topoShape2.getElementMapSize(), 0);
    EXPECT_EQ(_common->Shape.getShape().getElementMapSize(), 26);
}
