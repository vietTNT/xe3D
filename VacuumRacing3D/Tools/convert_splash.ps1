param($src='D:\VacuumRacing3D\VacuumRacing3D\Assets\Textures\splash.bmp')
try {
  Add-Type -AssemblyName System.Drawing
  $tmp = Join-Path (Split-Path $src) 'splash_converted.bmp'
  $img = [System.Drawing.Image]::FromFile($src)
  $bmp = New-Object System.Drawing.Bitmap $img
  $bmp.Save($tmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
  $bmp.Dispose(); $img.Dispose()
  Move-Item -Force $tmp $src
  Write-Host 'CONVERT_OK'
} catch {
  Write-Host 'CONVERT_ERR'
  Write-Host $_.Exception.Message
  exit 1
}
