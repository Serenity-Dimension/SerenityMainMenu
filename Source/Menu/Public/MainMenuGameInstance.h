// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "JsonUtilities.h"
#include "WebBrowser.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Blueprint/UserWidget.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserCookieManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MainMenuGameInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class MENU_API UMainMenuGameInstance : public UGameInstance
{
	GENERATED_BODY()
protected:
     
public:
	UMainMenuGameInstance();

		virtual void Shutdown() override;

	UPROPERTY()
		FString AccessToken;

	UFUNCTION(BlueprintCallable)
        void CreateLogWindow(UWebBrowser* WebBrowser);

	UFUNCTION()
		FString ReadFile(FString FilePath);

	UPROPERTY()
		FString IdToken;

	UFUNCTION(BlueprintCallable)
		FString getNickname();

	UFUNCTION(BlueprintCallable)
		FString getEmail();

	UPROPERTY()
		FString RefreshToken;

	UPROPERTY()
		FTimerHandle RetrieveNewTokensHandle;
	
	UFUNCTION(BlueprintCallable)
		void SetCognitoTokens(FString NewAccessToken, FString NewIdToken, FString NewRefreshToken);
		
	UPROPERTY()
		FString Nickname = "Guest";
	UPROPERTY()
		FString Email;
		
private:
	FHttpModule* HttpModule;
	
	UPROPERTY()
		FString ApiUrl;
	
	UPROPERTY()
		FString LoginUrl;

	UPROPERTY()
		FString CallbackUrl;
	UPROPERTY()
		UWebBrowser* m_WebBrowser;
	UFUNCTION()
		void RetrieveNewTokens();

	void OnRetrieveNewTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
	UFUNCTION()
		void HandleLoginUrlChange();
	
	void OnExchangeCodeForTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
