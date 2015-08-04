use strict;

my @RESOURCES = ("01_dejavusans.ttf", "02_droidsans-fallback.ttf", "03_jomolhari-id-a3d.ttf", "04_padauk.ttf", "05_khmeros.ttf", "06_code2000.ttf",
                 "basic.skn", "symbols_24.png",
                 "classificator.txt", "drawing_rules.bin",
                 "fonts_blacklist.txt", "fonts_whitelist.txt", "unicode_blocks.txt",
                 "languages.txt", "maps.update", "countries.txt", 
                 "welcome.html", "about-travelguide-desktop.html", "eula.html",
                 "dictionary.slf");

my @QT_LIBS = ("QtCore4.dll", "QtGui4.dll", "QtOpenGL4.dll", "QtNetwork4.dll", "QtWebkit4.dll");

my $QT_PATH = "..\\..\\..\\SDK\\Desktop\\Qt\\4.7.3\\msvc2010\\bin\\";
my $BINARY_PATH = "..\\..\\..\\omim-build-msvc2010\\out\\release\\";
my $DATA_PATH = "..\\..\\data\\";
my $MERGE_MODULE_PATH = "\\Program Files (x86)\\Common Files\\Merge Modules\\Microsoft_VC100_CRT_x86.msm";
my $MERGE_MODULE_TITLE = "Visual C++ 10.0 Runtime";

# read guids from file
my $GUIDS_FILE="guids.txt";
my %GUIDS;
open(IN, "<$GUIDS_FILE") or die "Couldn't open $GUIDS_FILE: $!";
while (<IN>)
{
  chomp;
  $_ =~ m/([0-9a-zA-Z-]*) (.*$)/;
  $GUIDS{$2} = $1;
}
close IN;

# generate mwm data file components list
sub GenComponents(\@$)
{
  my @files = @{(shift)};
  my $SOURCE_PATH = shift;
  my $COMPONENTS = "";
  my $REFS = "";
  foreach (@files)
  {
    $_ =~ /([a-zA-Z- _\.0-9]*)$/;
    my $nameOnly = $1;
    # fix id to be in valid wix format
    my $id = "_" . $nameOnly;
    $id =~ s/-/_/g;
    if (exists($GUIDS{$nameOnly}))
    {
      $COMPONENTS = $COMPONENTS . "             <Component Id='$id' Guid='$GUIDS{$nameOnly}'>\n" .
                                  "               <File Id='$id' Name='$nameOnly' Source='${SOURCE_PATH}$nameOnly' DiskId='1' KeyPath='yes' Checksum='no' />\n" .
                                  "             </Component>\n";
      $REFS = $REFS . "         <ComponentRef Id='$id' />\n";
    }
    else
    {
      # do nothing
      print "ERROR: missing file: $nameOnly\n";
    }
  }
  chomp($COMPONENTS);
  
  return ($COMPONENTS, $REFS);
}

my @files = <../../data/*.mwm>;
my @DATA_COMPONENTS = GenComponents(@files, $DATA_PATH);

my @RESOURCE_COMPONENTS = GenComponents(@RESOURCES, $DATA_PATH);

my @QT_COMPONENTS = GenComponents(@QT_LIBS, $QT_PATH);

print <<RAWTEXT;
<?xml version='1.0'?>
<Wix xmlns='http://schemas.microsoft.com/wix/2006/wi'>
  <Product Id='42180640-750C-4d9e-9087-519705C069D5'
           Name='MapsWithMe'
           Language='1033'
           Version='1.0.0'
           Manufacturer='MapsWithMe'
           UpgradeCode='DFCB23C7-99B3-4228-93E5-625C48370982'>
    
    <Package Description='MapsWithMe - offline maps and travel guide'
             Comments='Supports Windows XP SP3 and above'
             Manufacturer='MapsWithMe'
             InstallerVersion='300'
             Compressed='yes'
             InstallPrivileges='elevated'
             InstallScope='perMachine'
             Platform='x86' />
 
      <Media Id='1' Cabinet='data.cab' EmbedCab='no' CompressionLevel='high' />

      <Directory Id='TARGETDIR' Name='SourceDir'>
        <Directory Id='ProgramFilesFolder' Name='PFiles'>
          <Directory Id='MapsWithMeDir' Name='MapsWithMe'>
            <Component Id='MapsWithMe.exe' Guid='67852405-8C7C-4ec4-81E7-698CE3CD9A67'>
              <File Id='MapsWithMe.exe' Name='MapsWithMe.exe' Source='${BINARY_PATH}MapsWithMe.exe' DiskId='1' KeyPath='yes' Checksum='yes' />
            </Component>
$QT_COMPONENTS[0]
            <Directory Id='MWMDataDir' Name='Data'>
$DATA_COMPONENTS[0]
$RESOURCE_COMPONENTS[0]
            </Directory>
          </Directory>
        </Directory>
      </Directory>
 
      <Feature Id='MapsWithMeFeature' Title='MapsWithMe' Level='1'>
         <ComponentRef Id='MapsWithMe.exe' />
$QT_COMPONENTS[1]
$DATA_COMPONENTS[1]
$RESOURCE_COMPONENTS[1]
      </Feature>
      
      <DirectoryRef Id='TARGETDIR'>
        <Merge Id='VCRedist' SourceFile='$MERGE_MODULE_PATH' DiskId='1' Language='0' FileCompression='Yes'/>
      </DirectoryRef>
      <Feature Id='VCRedist' Title='$MERGE_MODULE_TITLE' AllowAdvertise='no' Display='hidden' Level='1'>
        <MergeRef Id='VCRedist'/>
      </Feature>
      
   </Product>
</Wix>


RAWTEXT
