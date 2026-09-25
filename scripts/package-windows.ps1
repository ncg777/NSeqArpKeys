param(
    [string] $OutputDirectory = '',
    [string] $BuildDirectory = 'build'
)

$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $repoRoot $BuildDirectory
}
$vstBundle = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'NSeqArpKeys_artefacts\Release\VST3\NSeqArpKeys.vst3'
$manifest = Join-Path $vstBundle 'Contents\Resources\moduleinfo.json'
$vstBinary = Join-Path $vstBundle 'Contents\x86_64-win\NSeqArpKeys.vst3'
$manual = Join-Path $repoRoot 'output\pdf\NSeqArpKeys-Manual.pdf'

foreach ($required in @($manifest, $vstBinary, $manual)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf) -or
        (Get-Item -LiteralPath $required).Length -eq 0) {
        throw "Required nonempty file is missing: $required"
    }
}

$pdfStream = [IO.File]::OpenRead($manual)
try {
    $header = New-Object byte[] 5
    if ($pdfStream.Read($header, 0, 5) -ne 5 -or
        [Text.Encoding]::ASCII.GetString($header) -ne '%PDF-') {
        throw "Manual is not a PDF: $manual"
    }
} finally {
    $pdfStream.Dispose()
}

$metadata = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
$version = [string] $metadata.Version
if ($metadata.Name -ne 'NSeqArpKeys' -or $version -notmatch '^\d+\.\d+\.\d+(?:[.-][A-Za-z0-9.-]+)?$') {
    throw "VST3 manifest has an unexpected name or version: $manifest"
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'release'
} elseif (-not [IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot $OutputDirectory
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null

$zipName = "NSeqArpKeys-$version-windows-x64.zip"
$finalZip = Join-Path $OutputDirectory $zipName
$temporaryZip = Join-Path $OutputDirectory ('.' + $zipName + '.' + [guid]::NewGuid().ToString('N') + '.tmp')
$backupZip = Join-Path $OutputDirectory ('.' + $zipName + '.' + [guid]::NewGuid().ToString('N') + '.bak')

$files = @()
$bundlePrefix = [IO.Path]::GetFullPath($vstBundle).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
foreach ($file in (Get-ChildItem -LiteralPath $vstBundle -File -Recurse | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($bundlePrefix.Length).Replace('\', '/')
    $files += [pscustomobject]@{ Source = $file.FullName; Entry = "NSeqArpKeys.vst3/$relative" }
}
$files += [pscustomobject]@{ Source = $manual; Entry = 'NSeqArpKeys-Manual.pdf' }

$startHere = @"
NSeqArpKeys $version - Windows x64

VST3: copy the entire NSeqArpKeys.vst3 folder to
  C:\Program Files\Common Files\VST3
Then rescan plug-ins in your DAW and load NSeqArpKeys as an instrument.

Open NSeqArpKeys-Manual.pdf for installation, controls, pattern editing,
Forte set search, presets, and troubleshooting.

The Pattern Bank includes twelve Fourth Atlas melodic and rhythmic banks
and a starter bank inside the plug-in.

Quick start: load the Single Note Pulse preset, then hold C4 (MIDI 60).
Release the key to stop. Enable Latch to hear the pattern while editing;
Stop Key and Stop All end playback.

SHA256SUMS.txt lists the SHA-256 checksums of the packaged files.
"@

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$encoding = New-Object Text.UTF8Encoding $false
$sha = [Security.Cryptography.SHA256]::Create()
$expected = @{}

function Add-TextEntry {
    param($Archive, [string] $Name, [string] $Content, $Encoding)
    $entry = $Archive.CreateEntry($Name, [IO.Compression.CompressionLevel]::Optimal)
    $stream = $entry.Open()
    try {
        $writer = New-Object IO.StreamWriter($stream, $Encoding)
        try { $writer.Write($Content) } finally { $writer.Dispose() }
    } finally {
        $stream.Dispose()
    }
}

try {
    $sums = New-Object 'System.Collections.Generic.List[string]'
    $archive = [IO.Compression.ZipFile]::Open($temporaryZip, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in $files) {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $file.Source, $file.Entry, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
            $hash = (Get-FileHash -LiteralPath $file.Source -Algorithm SHA256).Hash.ToLowerInvariant()
            $expected[$file.Entry] = $hash
            $sums.Add("$hash  $($file.Entry)")
        }
        Add-TextEntry $archive 'START-HERE.txt' $startHere $encoding
        $expected['START-HERE.txt'] = [BitConverter]::ToString(
            $sha.ComputeHash($encoding.GetBytes($startHere))).Replace('-', '').ToLowerInvariant()
        $sums.Add("$($expected['START-HERE.txt'])  START-HERE.txt")
        $sumsText = ($sums -join "`n") + "`n"
        Add-TextEntry $archive 'SHA256SUMS.txt' $sumsText $encoding
        $expected['SHA256SUMS.txt'] = [BitConverter]::ToString(
            $sha.ComputeHash($encoding.GetBytes($sumsText))).Replace('-', '').ToLowerInvariant()
    } finally {
        $archive.Dispose()
    }

    $archive = [IO.Compression.ZipFile]::OpenRead($temporaryZip)
    try {
        if ($archive.Entries.Count -ne $expected.Count) {
            throw "ZIP entry count mismatch: $($archive.Entries.Count) instead of $($expected.Count)"
        }
        foreach ($entry in $archive.Entries) {
            if (-not $expected.ContainsKey($entry.FullName)) {
                throw "Unexpected ZIP entry: $($entry.FullName)"
            }
            $stream = $entry.Open()
            try {
                $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
            } finally {
                $stream.Dispose()
            }
            if ($actual -ne $expected[$entry.FullName]) {
                throw "ZIP checksum mismatch: $($entry.FullName)"
            }
        }
    } finally {
        $archive.Dispose()
    }

    if (Test-Path -LiteralPath $finalZip -PathType Leaf) {
        [IO.File]::Move($finalZip, $backupZip)
        try {
            [IO.File]::Move($temporaryZip, $finalZip)
        } catch {
            [IO.File]::Move($backupZip, $finalZip)
            throw
        }
        [IO.File]::Delete($backupZip)
    } else {
        [IO.File]::Move($temporaryZip, $finalZip)
    }
    Write-Output "Created and verified: $finalZip"
    Write-Output "Files: $($expected.Count); Size: $((Get-Item -LiteralPath $finalZip).Length) bytes"
} finally {
    $sha.Dispose()
    if (Test-Path -LiteralPath $temporaryZip -PathType Leaf) {
        Remove-Item -LiteralPath $temporaryZip -Force
    }
}
