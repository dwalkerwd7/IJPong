// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Engine/World.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Meta/IJPSkillTree.h"
#include "Run/IJPRunMapView.h"
#include "Tests/IJPTestWorld.h"

namespace IJPMenuTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FIJPSkillNode MakeNode(const TCHAR* Id, const TCHAR* Parent, int32 Branch, int32 Level = 0)
	{
		FIJPSkillNode Node;
		Node.Id = Id;
		Node.Parent = Parent;
		Node.Branch = Branch;
		Node.Level = Level;
		return Node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTreeUpDownTest, "IJPong.Menus.UpAndDownMoveThroughTheTree", IJPMenuTests::Flags)
bool FIJPTreeUpDownTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	AIJPRunMapView* View = Mode->GetMapView();

	// A node at the top of each branch, and one under the middle branch's.
	UIJPSkillTree* Tree = NewObject<UIJPSkillTree>(GetTransientPackage(), TEXT("MenuTree"));
	Tree->Nodes.Add(IJPMenuTests::MakeNode(TEXT("Left"), TEXT(""), 0));
	Tree->Nodes.Add(IJPMenuTests::MakeNode(TEXT("Mid"), TEXT(""), 1));
	Tree->Nodes.Add(IJPMenuTests::MakeNode(TEXT("Right"), TEXT(""), 2));
	Tree->Nodes.Add(IJPMenuTests::MakeNode(TEXT("MidDeep"), TEXT("Mid"), 1, 1));
	View->ShowTree(Tree, true);

	for (int32 i = 0; i < 10 && View->GetSelectedTreeNode() != 1; ++i)
	{
		View->Step(1);
	}
	UTEST_EQUAL("On the middle branch's top", View->GetSelectedTreeNode(), 1);

	View->StepVertical(-1);
	UTEST_EQUAL("Down: the node below it", View->GetSelectedTreeNode(), 3);
	View->StepVertical(-1);
	UTEST_EQUAL("Down again: START RUN", View->GetSelectedTreeNode(), static_cast<int32>(INDEX_NONE));
	View->StepVertical(-1);
	UTEST_EQUAL("Nothing lower: stays", View->GetSelectedTreeNode(), static_cast<int32>(INDEX_NONE));
	View->StepVertical(1);
	UTEST_EQUAL("Up: back to the deep node", View->GetSelectedTreeNode(), 3);
	View->StepVertical(1);
	UTEST_EQUAL("Up: its parent", View->GetSelectedTreeNode(), 1);

	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	return true;
}

#endif
