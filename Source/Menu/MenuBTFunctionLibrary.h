// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "iostream"
#include "MyBTFunctionLibrary.h"
#include "MenuBTFunctionLibrary.generated.h"

using namespace std;


/**
 * 
 */
UCLASS()
class MENU_API UMenuBTFunctionLibrary : public UMyBTFunctionLibrary
{
	GENERATED_BODY()
public:

UFUNCTION(BlueprintCallable)
	static void MovePawn(APawn* me, float speed, FVector position);
};
