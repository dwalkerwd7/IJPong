// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPPaddleClass.generated.h"

class UIJPAbility;
class UIJPPaddleProfile;

/**
 * A paddle class: who the paddle is. Its base stats (a profile), its fixed class skill, and later
 * its sprite and skill tree. Profiles stay pure stats, so a tree upgrade or a tougher rival can
 * swap in another profile without being another class.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPPaddleClass : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	FText DisplayName;

	/** Size and movement. Empty = UIJPPaddleProfile's defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TObjectPtr<UIJPPaddleProfile> Profile;

	/** Always in the class-skill slot. Empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TObjectPtr<UIJPAbility> ClassSkill;
};
