// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameInstance.h"


UMainMenuGameInstance::UMainMenuGameInstance() {
}

FString UMainMenuGameInstance::ReadFile(FString FilePath) {
	FString DirectoryPath = FPaths::ProjectContentDir();
	FString FullPath = DirectoryPath + "/" + FilePath;
	FString Result;
	IPlatformFile& File = FPlatformFileManager::Get().GetPlatformFile();

	if (File.FileExists(*FullPath)) {
		FFileHelper::LoadFileToString(Result, *FullPath);
	}

	return Result;
}

void UMainMenuGameInstance::CreateLogWindow(UWebBrowser* WebBrowser) {
	m_WebBrowser = WebBrowser;
	ApiUrl = UMainMenuGameInstance::ReadFile("Url/ApiUrl.txt");
	HttpModule = &FHttpModule::Get();
	bool bIsFocusable = true;
	IWebBrowserSingleton* WebBrowserSingleton = IWebBrowserModule::Get().GetSingleton();
	if (WebBrowserSingleton != nullptr) {
		TOptional<FString> DefaultContext;
		TSharedPtr<IWebBrowserCookieManager> CookieManager = WebBrowserSingleton->GetCookieManager(DefaultContext);
		if (CookieManager.IsValid()) {
			CookieManager->DeleteCookies();
		}
	}
	WebBrowser->LoadURL(UMainMenuGameInstance::ReadFile("Url/LoginUrl.txt"));
	FScriptDelegate LoginDelegate;
	LoginDelegate.BindUFunction(this, "HandleLoginUrlChange");
	WebBrowser->OnUrlChanged.Add(LoginDelegate);
}

void UMainMenuGameInstance::Shutdown() {
	Super::Shutdown();
	if (AccessToken.Len() > 0) {
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> InvalidateTokensRequest = HttpModule->CreateRequest();
		InvalidateTokensRequest->SetURL(ApiUrl + "invalidatetokens");
		InvalidateTokensRequest->SetVerb("GET");
		InvalidateTokensRequest->SetHeader("Content-Type", "application/json");
		InvalidateTokensRequest->SetHeader("Authorization", AccessToken);
		InvalidateTokensRequest->ProcessRequest();
	}
}

void UMainMenuGameInstance::SetCognitoTokens(FString NewAccessToken, FString NewIdToken, FString NewRefreshToken) {
	AccessToken = NewAccessToken;
	IdToken = NewIdToken;
	RefreshToken = NewRefreshToken;
	GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UMainMenuGameInstance::RetrieveNewTokens, 1.0f, false, 3300.0f);
}

void UMainMenuGameInstance::RetrieveNewTokens() {
	if (AccessToken.Len() > 0 && RefreshToken.Len() > 0) {
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("refreshToken", RefreshToken);
		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RetrieveNewTokensRequest = HttpModule->CreateRequest();
			RetrieveNewTokensRequest->OnProcessRequestComplete().BindUObject(this, &UMainMenuGameInstance::OnRetrieveNewTokensResponseReceived);
			RetrieveNewTokensRequest->SetURL(ApiUrl + "/retrievenewtokens");
			RetrieveNewTokensRequest->SetVerb("POST");
			RetrieveNewTokensRequest->SetHeader("Content-Type", "application/json");
			RetrieveNewTokensRequest->SetHeader("Authorization", AccessToken);
			RetrieveNewTokensRequest->SetContentAsString(RequestBody);
			RetrieveNewTokensRequest->ProcessRequest();
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UMainMenuGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
		}
	}
}

void UMainMenuGameInstance::OnRetrieveNewTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	if (bWasSuccessful) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
			if (!JsonObject->HasField("error")) {
				SetCognitoTokens(JsonObject->GetStringField("accessToken"), JsonObject->GetStringField("idToken"), RefreshToken);
			}
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UMainMenuGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
		}
	}
	else {
		GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UMainMenuGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
	}
}

void UMainMenuGameInstance::HandleLoginUrlChange() {
	FString BrowserUrl = m_WebBrowser->GetUrl();
	FString Url;
	FString QueryParameters;
	if (BrowserUrl.Split("?", &Url, &QueryParameters)) {
		if (Url.Equals(UMainMenuGameInstance::ReadFile("Url/CallbackUrl.txt"))) {
			FString ParameterName;
			FString ParameterValue;
			if (QueryParameters.Split("=", &ParameterName, &ParameterValue)) {
				if (ParameterName.Equals("code")) {
					ParameterValue = ParameterValue.Replace(*FString("#"), *FString(""));
					TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
					RequestObj->SetStringField(ParameterName, ParameterValue);
					FString RequestBody;
					TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
					if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
						TSharedRef<IHttpRequest, ESPMode::ThreadSafe> ExchangeCodeForTokensRequest = HttpModule->CreateRequest();
						ExchangeCodeForTokensRequest->OnProcessRequestComplete().BindUObject(this, &UMainMenuGameInstance::OnExchangeCodeForTokensResponseReceived);
						ExchangeCodeForTokensRequest->SetURL(ApiUrl + "/exchangecodefortokens");
						ExchangeCodeForTokensRequest->SetVerb("POST");
						ExchangeCodeForTokensRequest->SetHeader("Content-Type", "application/json");
						ExchangeCodeForTokensRequest->SetContentAsString(RequestBody);
						ExchangeCodeForTokensRequest->ProcessRequest();
					}
				}
			}
		}
	}
}

void UMainMenuGameInstance::OnExchangeCodeForTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful){
	if (bWasSuccessful) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
			if (!JsonObject->HasField("error")) {
						FString l_AccessToken = JsonObject->GetStringField("access_token");
						FString l_IdToken = JsonObject->GetStringField("id_token");
						FString l_RefreshToken = JsonObject->GetStringField("refresh_token");
						SetCognitoTokens(l_AccessToken, l_IdToken, l_RefreshToken);
						TSharedRef<IHttpRequest, ESPMode::ThreadSafe> GetPlayerDataRequest = HttpModule->CreateRequest();
						GetPlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &UMainMenuGameInstance::OnGetPlayerDataResponseReceived);
						GetPlayerDataRequest->SetURL(ApiUrl + "/getplayerdata");
						GetPlayerDataRequest->SetVerb("GET");
						GetPlayerDataRequest->SetHeader("Authorization", AccessToken);
						GetPlayerDataRequest->ProcessRequest();
					}
				}
			}
		}

void UMainMenuGameInstance::OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	if (bWasSuccessful) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
			if (!JsonObject->HasField("error")) {
				TSharedPtr<FJsonObject> PlayerData = JsonObject->GetObjectField("playerData");
				TSharedPtr<FJsonObject> NicknameObject = PlayerData->GetObjectField("Nickname");
				TSharedPtr<FJsonObject> EmailObject = PlayerData->GetObjectField("Email");
				Nickname = NicknameObject->GetStringField("S");
				Email = EmailObject->GetStringField("S");
				m_WebBrowser->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

FString UMainMenuGameInstance::getNickname() {
	return Nickname;
}

FString UMainMenuGameInstance::getEmail() {
	return Email;
}