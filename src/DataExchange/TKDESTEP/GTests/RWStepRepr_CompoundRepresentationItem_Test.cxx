// Copyright (c) 2026 Touko Ursin
//
// This file is part of Open CASCADE Technology software library.
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in the
// OCCT distribution for complete text of the license and disclaimer of warranty.

#include <IFSelect_PrintCount.hxx>
#include <STEPControl_Reader.hxx>
#include <StepData_StepModel.hxx>
#include <StepRepr_CompoundRepresentationItem.hxx>
#include <StepRepr_DescriptiveRepresentationItem.hxx>
#include <sstream>
#include <string>
#include <gtest/gtest.h>

namespace
{
std::string stepContent(const char* theElements)
{
  return std::string("ISO-10303-21;\nHEADER;\n"
                     "FILE_DESCRIPTION(('Compound reader regression'),'2;1');\n"
                     "FILE_NAME('compound.step','',(''),(''),'','','');\n"
                     "FILE_SCHEMA(('AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF'));\n"
                     "ENDSEC;\nDATA;\n#1=COMPOUND_REPRESENTATION_ITEM('modifiers',")
         + theElements
         + ");\n"
           "#2=DESCRIPTIVE_REPRESENTATION_ITEM('dimensional note','statistical');\n"
           "#3=DESCRIPTIVE_REPRESENTATION_ITEM('dimensional note','continuous feature');\n"
           "ENDSEC;\nEND-ISO-10303-21;\n";
}

occ::handle<StepRepr_CompoundRepresentationItem> loadCompound(STEPControl_Reader& theReader,
                                                              const char*         theElements)
{
  std::istringstream aStream(stepContent(theElements));
  EXPECT_EQ(theReader.ReadStream("compound.step", aStream), IFSelect_RetDone);
  const auto aModel = theReader.StepModel();
  if (aModel.IsNull())
    return nullptr;
  for (int anIndex = 1; anIndex <= aModel->NbEntities(); ++anIndex)
  {
    auto anItem = occ::down_cast<StepRepr_CompoundRepresentationItem>(aModel->Entity(anIndex));
    if (!anItem.IsNull())
      return anItem;
  }
  return nullptr;
}

void expectTwoItems(const char* theElements, const char* theFirst, const char* theSecond)
{
  STEPControl_Reader aReader;
  const auto         anItem = loadCompound(aReader, theElements);
  ASSERT_FALSE(anItem.IsNull());
  ASSERT_FALSE(anItem->ItemElement().IsNull());
  ASSERT_EQ(anItem->NbItemElement(), 2);
  const auto aFirst =
    occ::down_cast<StepRepr_DescriptiveRepresentationItem>(anItem->ItemElementValue(1));
  const auto aSecond =
    occ::down_cast<StepRepr_DescriptiveRepresentationItem>(anItem->ItemElementValue(2));
  ASSERT_FALSE(aFirst.IsNull());
  ASSERT_FALSE(aSecond.IsNull());
  EXPECT_STREQ(aFirst->Description()->ToCString(), theFirst);
  EXPECT_STREQ(aSecond->Description()->ToCString(), theSecond);
  std::ostringstream aChecks;
  aReader.PrintCheckLoad(aChecks, true, IFSelect_ItemsByEntity);
  EXPECT_EQ(aChecks.str().find("COMPOUND_REPRESENTATION_ITEM"), std::string::npos);
}
} // namespace

TEST(RWStepRepr_CompoundRepresentationItem, TypedSetElements)
{
  expectTwoItems("SET_REPRESENTATION_ITEM((#2,#3))", "statistical", "continuous feature");
}

TEST(RWStepRepr_CompoundRepresentationItem, TypedListPreservesOrder)
{
  expectTwoItems("LIST_REPRESENTATION_ITEM((#3,#2))", "continuous feature", "statistical");
}

TEST(RWStepRepr_CompoundRepresentationItem, LegacyAggregateStillReadable)
{
  expectTwoItems("(#3,#2)", "continuous feature", "statistical");
}

TEST(RWStepRepr_CompoundRepresentationItem, TypedScalarIsDiagnosed)
{
  STEPControl_Reader aReader;
  const auto         anItem = loadCompound(aReader, "SET_REPRESENTATION_ITEM(#2)");
  ASSERT_FALSE(anItem.IsNull());
  EXPECT_TRUE(anItem->ItemElement().IsNull());
  std::ostringstream aChecks;
  aReader.PrintCheckLoad(aChecks, true, IFSelect_ItemsByEntity);
  EXPECT_NE(aChecks.str().find("not a LIST"), std::string::npos);
}

TEST(RWStepRepr_CompoundRepresentationItem, ExtraTypedParameterIsDiagnosed)
{
  STEPControl_Reader aReader;
  const auto         anItem = loadCompound(aReader, "SET_REPRESENTATION_ITEM((#2),(#3))");
  ASSERT_FALSE(anItem.IsNull());
  EXPECT_TRUE(anItem->ItemElement().IsNull());
  std::ostringstream aChecks;
  aReader.PrintCheckLoad(aChecks, true, IFSelect_ItemsByEntity);
  EXPECT_NE(aChecks.str().find("Count of Parameters"), std::string::npos);
}
