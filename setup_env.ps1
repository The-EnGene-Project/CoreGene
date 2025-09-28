# Titulo da janela do terminal
$Host.UI.RawUI.WindowTitle = "Terminal para Comp-Graf (MinGW)"

# Mensagem de boas-vindas
Write-Host "Configurando o ambiente para MinGW no PowerShell..." -ForegroundColor Green

# --- A LINHA MAIS IMPORTANTE ---
# Adiciona (prepends) o caminho do bin do MinGW à variável PATH desta sessão
$env:Path = "C:\msys64\ucrt64\bin;" + $env:Path

# Mensagem de sucesso
Write-Host "Ambiente pronto! O compilador MinGW (gcc/g++) esta no PATH." -ForegroundColor Green
Write-Host ""