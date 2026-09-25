// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPPaddleClass.generated.h"

class UTexture2D;
class UIJPAbility;
class UIJPPaddleProfile;
class UIJPSkillTree;

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

	/** The paddle's sprite (greyscale, tinted by the era), shown in eras with sprites. Empty = a plain rectangle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class|Look")
	TObjectPtr<UTexture2D> Sprite;

	/** Each end cap's share of the sprite's height: those stay fixed while the middle stretches to the paddle's length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class|Look", meta = (ClampMin = "0", ClampMax = "0.5"))
	float SpriteCap = 0.1f;

	/** Grown between runs with the meta currencies. Empty = no tree yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TObjectPtr<UIJPSkillTree> SkillTree;
};
