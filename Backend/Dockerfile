# Bygger TamaPal-backenden og lægger Andriis frontend i wwwroot, så siden og /gamehub ligger på samme adresse.
# Build context skal være roden af repoet (Docker Context Path "." i Dokploy), så Frontend/ kan kopieres med.

# Trin 1: Byg backenden med .NET SDK
FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build
WORKDIR /src

COPY ["Backend/BackendTamapal/BackendTamapal.csproj", "Backend/BackendTamapal/"]
RUN dotnet restore "Backend/BackendTamapal/BackendTamapal.csproj"

COPY ["Backend/BackendTamapal/", "Backend/BackendTamapal/"]
WORKDIR /src/Backend/BackendTamapal
RUN dotnet publish "BackendTamapal.csproj" -c Release -o /app/publish /p:UseAppHost=false

# Trin 2: Kør den færdige app med det mindre runtime-image
FROM mcr.microsoft.com/dotnet/aspnet:10.0 AS final
WORKDIR /app
EXPOSE 8080
ENV ASPNETCORE_URLS=http://+:8080
# Skolens server har brugt sine inotify-instanser op, så .NET må ikke overvåge filer med inotify
ENV DOTNET_HOSTBUILDER__RELOADCONFIGONCHANGE=false
ENV DOTNET_USE_POLLING_FILE_WATCHER=true

COPY --from=build /app/publish .
COPY ["Frontend/", "./wwwroot/"]

ENTRYPOINT ["dotnet", "BackendTamapal.dll"]
