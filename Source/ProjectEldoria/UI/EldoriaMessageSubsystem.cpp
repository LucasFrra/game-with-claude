// Project Eldoria

#include "UI/EldoriaMessageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ProjectEldoria.h"

void UEldoriaMessageSubsystem::ShowMessage(const UObject* WorldContextObject, const FText& Message, float Duration)
{
	if (!GEngine || Message.IsEmpty())
	{
		return;
	}

	UE_LOG(LogProjectEldoria, Log, TEXT("[Message] %s"), *Message.ToString());
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (UEldoriaMessageSubsystem* Subsystem = World ? World->GetSubsystem<UEldoriaMessageSubsystem>() : nullptr)
	{
		Subsystem->OnMessageShown.Broadcast(Message, Duration);
	}
}

bool UEldoriaMessageSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
