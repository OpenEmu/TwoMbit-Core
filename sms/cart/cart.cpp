
#include "cart.h"
#include "../system/system.h"
#include "../bus/bus.h"
#include "hl_md5.h"

#include <algorithm>


Cart::~Cart() {
    unload();
}

Cart::Cart(_System* sys, Bus& bus)
:
sys(sys),
bus(bus)
{
    mapperHintSize = sizeof( mapperHint) / sizeof( mapperHint[0] );
    backupRamInfoSize = sizeof( backupRamInfo) / sizeof( backupRamInfo[0] );
    biosInfoSize = sizeof( biosInfo) / sizeof( biosInfo[0] );
    smsModeInfoSize = sizeof( smsModeInfo) / sizeof( smsModeInfo[0] );
    glassesInfoSize = sizeof( glassesInfo) / sizeof( glassesInfo[0] );
    cartWramInfoSize = sizeof( cartWramInfo) / sizeof( cartWramInfo[0] );
    wramPatternSize = sizeof( wramPattern) / sizeof( wramPattern[0] );
    customMapperSize = sizeof( customMapper) / sizeof( customMapper[0] );
    palRegionsSize = sizeof( palRegions ) / sizeof( palRegions[0] );
    disablePort3eSize = sizeof( disablePort3e ) / sizeof( disablePort3e[0] );
    sgWram8KSize = sizeof( sgWram8K ) / sizeof( sgWram8K[0] );

    //pal games
    palRegions[0].hash = "09202BC26FCF19EBD3FFCBD80E23ED9D"; //addams familiy
    palRegions[1].hash = "5401DDCC788C0F1A5DFE26E56F239FBF"; //battle maniacs
    palRegions[2].hash = "4B3C934B3DAA99A6B341DA57388A7543"; //bram stocker dracula
    palRegions[3].hash = "672EE23F7D4C373F1E537583865753FE"; //bram stocker dracula
    palRegions[4].hash = "5AC58B3A7FDA23161AB1185B3EE797AC"; //california games 2
    palRegions[5].hash = "5EA2D1C22EF243DBA3125E65FAB67876"; //california games 2
    palRegions[6].hash = "E50336C0D146E8A341C7AE198A016527"; //lion king
    palRegions[7].hash = "1389419D90834D3700B772E984143FDE"; //predator 2
    palRegions[8].hash = "58B89D62438407F242BBE713F1D945CA"; //ren & stimpy
    palRegions[9].hash = "C656DC9F901387CC51276E4709C340F8"; //robocop 3
    palRegions[10].hash = "5CA0A2B33DB8EEDCDDB1D5AEAA8EA5A4"; //sensible soccer
    palRegions[11].hash = "C2055D5C66DD3944B99642C18032EBC4"; //shadow dancer
    palRegions[12].hash = "9B54B7FC5E7EA524E8FD59AAB538286E"; //shadow dancer
    palRegions[13].hash = "ABC8A9E43C588C7D0535CC8305BDBF94"; //shadow of the beast
    palRegions[14].hash = "CE7F314A657E0F0FE506FB7AEA016F31"; //sonic blast
    palRegions[15].hash = "1F1CE1D74C416F2B85CC3F24D316F9E3"; //xenon2
    palRegions[16].hash = "27FBA4B7DB989C663DDA11C255CF49F3"; //xenon2
    palRegions[17].hash = "C59388F474B2500983315536687D230F"; //xenon2
    palRegions[18].hash = "B0100AD9F1838332B15D0434BFA9FE29"; //home alone
    palRegions[19].hash = "3D24A52E98E6C85D7C059386451CE749"; //back to the future 3
    palRegions[20].hash = "F2535DF9BDC3A84221303FA62D61AD6E"; //back to the future 2
    palRegions[21].hash = "26DF4404950CB8DA47235833C0C101C6"; //bart vs space mutants
    palRegions[22].hash = "08511C5CF0DF0941B10EBF28050AFE51"; //chase HQ
    palRegions[23].hash = "6D301C44CA739231D72AC8DDCE8A1B2B"; //chase HQ
    palRegions[24].hash = "7DEBF96F69C896D8571A6AD6CFACF502"; //cosmic spacehead
    palRegions[25].hash = "DF89AA1EBC5F42DB21FB8D12D9E78511"; //dessert strike
    palRegions[26].hash = "7AEF58645E419BB24BFA63239D2F36C7"; //dessert strike
    palRegions[27].hash = "7CA6753F8229FD5F6C6AD29768913A09"; //dessert strike
    palRegions[28].hash = "B8EB0CB6A9D16CFD08D9C03297FCD445"; //dizzy
    palRegions[29].hash = "C52E496C497D427747769AAEDFBB8DAB"; //james bond
    palRegions[30].hash = "17F9C50C444DC94E08BA173AB306608B"; //laser ghost
    palRegions[31].hash = "78864F17E5BC682AD0622AD046A461B3"; //laser ghost
    palRegions[32].hash = "A90CF8D68FA8B1F0BA00F0DDB9D547B7"; //micro machines
    palRegions[33].hash = "2629D3BFFD129834A3F5E033D2E36609"; //micro machines
    palRegions[34].hash = "05E5054E2EA818D4B7EB53D624562DCF"; //new zealand story
    palRegions[35].hash = "8C5A9E903A3B7D527679BDACFAAED388"; //new zealand story
    palRegions[36].hash = "2CA2064302F51F724E1F2593369A0696"; //operation wolf
    palRegions[37].hash = "71185DF29D8E42B675A84CCA8760DD91"; //operation wolf
    palRegions[38].hash = "7C432022FB0B73907F18B9F9769E2330"; //operation wolf
    palRegions[39].hash = "DA53D18840D3197313155C85FFDE9871"; //bank panic
    palRegions[40].hash = "9D39C9FB0547D1411750B15F8220810F"; //bank panic
    palRegions[41].hash = "F86E099CA8E407A65A7751D84C2EE519"; //dragon
    palRegions[42].hash = "2F75611B233C574C10EEADD5E07661BB"; //dragon
    palRegions[43].hash = "86806BBA2ACF7A07AAD125F6EF02CD51"; //smurfs
    palRegions[44].hash = "FE2AA0402EB30CEC17B3BF4504F8219B"; //smurfs
    palRegions[45].hash = "CC2454EEBBBFAB7BA99851A923DFDAAF"; //terminator 2
    //3d games
    glassesInfo[0].hash = "A20798B12481284F77CEE67CE5E25F2B"; //Blade Eagle 3D
    glassesInfo[1].hash = "7F33FA925C9EDE3F5BE2C7BA186A917C";
    glassesInfo[2].hash = "75F3DF352AA887BAF8DF455188AC853A";
    glassesInfo[3].hash = "1597AD705990A66E90262EF7B1141B64"; //Line of Fire
    glassesInfo[4].hash = "2349EE42A09088A4A590E2C65474CDAF"; //Maze Hunter 3D
    glassesInfo[5].hash = "B17CA24555CC79C5F7D89F156B99E452"; //Missile Defense 3D
    glassesInfo[6].hash = "558C793AAB09B46BED78431348191060"; //Outrun 3D
    glassesInfo[7].hash = "3586416AC6F2AA1FA7DEA351AFB8AC1C"; //Poseidon Wars 3D
    glassesInfo[8].hash = "D8717EEB943C292C078429A0ED5A8527"; //Space Harrier 3D
    glassesInfo[9].hash = "945A4202181ABE8C50082A65211EF7C7";
    glassesInfo[10].hash = "177EAAA2CAB1DF16A6B9A372D7D4846A"; //Zaxxon 3D
    glassesInfo[11].hash = "882DC10B0792BE579A745C3F38010642";
    //bios
    biosInfo[0].hash = "24A519C53F67B00640D0048EF7089105"; //SMS Bios v2.1 (jap)
    biosInfo[0].region = JAPAN;
    biosInfo[1].hash = "E49E63328C78FE3D24AE32BF903583E0"; //SMS Bios, Alex Kid built in (korean)
    biosInfo[1].region = JAPAN;
    biosInfo[2].hash = "3153447D155FB8BFC66A4D1C41E399B9"; //SMS prototype 404 Bios (us)
    biosInfo[2].region = USA;
    biosInfo[3].hash = "840481177270D5642A14CA71EE72844C"; //SMS Bios 1.3 (ue, eu)
    biosInfo[3].region = USA;
    biosInfo[4].hash = "02CBB2E348945C9AC41E37502A58CA76"; //SMS Bios v2.4, Hang On and Safari Hunt built in (us, eu)
    biosInfo[4].region = USA;
    biosInfo[5].hash = "5CD8F62CD8786AF0226E6D2248279338"; //SMS Bios v3.4, Hang On built in (us, eu)
    biosInfo[5].region = USA;
    biosInfo[6].hash = "08B81AA6BE18B92DAEF1B875DEECF824"; //SMS Bios v4.4, Missle Defense 3D built in (us, eu)
    biosInfo[6].region = USA;
    biosInfo[7].hash = "E8B26871629B938887757A64798DF6DC"; //SMS Bios, Alex Kid built in (us, eu)
    biosInfo[7].region = USA;
    biosInfo[8].hash = "4187D96BEAF36385E681A3CF3BD1663D"; //SMS Bios, Sonic built in (eu)
    biosInfo[8].region = EUROPE;
    biosInfo[9].hash = "672E104C3BE3A238301ACEFFC3B23FD6"; //GameGear Bios
    biosInfo[9].region = JAPAN;

    //custom sram sizes which are different from the standard 8k ones
    backupRamInfo[0].hash = "B0C35BC53AB7C184D34E5624F69AAD24"; //The Majors Pro Baseball
    backupRamInfo[0].sramSize = 128;
    backupRamInfo[1].hash = "E7EABBFC7A1F1339C4720249AEA92A32"; //World Series Basebal 1.0
    backupRamInfo[1].sramSize = 128;
    backupRamInfo[2].hash = "59359FC38865CFF00C90D6EB148DDC2F"; //World Series Basebal 1.1
    backupRamInfo[2].sramSize = 128;
    backupRamInfo[3].hash = "05CAC33029F0CAAC27774504C1AA8597"; //World Series Basebal '95
    backupRamInfo[3].sramSize = 128;
    backupRamInfo[4].hash = "E9F8ABB1EEA7FB9D7F3FDF4EFEC986A1"; //Nomo Hideo no World Series Baseball
    backupRamInfo[4].sramSize = 128;
    backupRamInfo[5].hash = "F9AE1762ED006C001E811FE6F072ABB7"; //Pro Yakyuu GG League
    backupRamInfo[5].sramSize = 128;
    backupRamInfo[6].hash = "45D214CD027DEE5CE2ADFCD6458DEC2C"; //Pro Yakyuu GG League '94
    backupRamInfo[6].sramSize = 128;
    backupRamInfo[7].hash = "95FB42E84A2B08ACCE481B0894EA3CEA"; //Moldorian
    backupRamInfo[7].sramSize = 24 * 1024;
    backupRamInfo[8].hash = "8EAFD80D35251B5D3F07D5CAE27241C1"; //Shining Force Gaiden
    backupRamInfo[8].sramSize = 16 * 1024;
    backupRamInfo[9].hash = "9540516E463958E7C24DDB0A92C9D329"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[9].sramSize = 32 * 1024;
    backupRamInfo[10].hash = "9540516E463958E7C24DDB0A92C9D329"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[10].sramSize = 32 * 1024;
    backupRamInfo[11].hash = "534435CBA54B68BACED0DCECAA7D0B69"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[11].sramSize = 32 * 1024;
    backupRamInfo[12].hash = "DC87296C5A01CD68A422041F579EB753"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[12].sramSize = 32 * 1024;
    backupRamInfo[13].hash = "95B29E16DF53A4EDF8A35A3E9A3075D9"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[13].sramSize = 32 * 1024;
    backupRamInfo[14].hash = "53D3C3639659EAA44904545D99E7CCFC"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[14].sramSize = 32 * 1024;
    backupRamInfo[15].hash = "4E03F12537D037907E6DDB26F4B8E5F9"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[15].sramSize = 32 * 1024;
    backupRamInfo[16].hash = "ED953655756BEFF5D97560FC3C1419D6"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[16].sramSize = 32 * 1024;
    backupRamInfo[17].hash = "FF1367CDF734182A28E1F47E8A498E2C"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[17].sramSize = 32 * 1024;
    backupRamInfo[18].hash = "521BFB56D184E0EE5F848D5DC28A8253"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[18].sramSize = 32 * 1024;
    backupRamInfo[19].hash = "35EF27F6B4B4DDC4A6AD6C78DB8C890B"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[19].sramSize = 32 * 1024;
    backupRamInfo[20].hash = "8857422E565412A59C77CB29550715E4"; //Shining Force Gaiden 2
    backupRamInfo[20].sramSize = 32 * 1024;
    backupRamInfo[21].hash = "0A0FA9CBCC3DB467191E907794C8C02B"; //Shining Force Gaiden 2
    backupRamInfo[21].sramSize = 32 * 1024;
    backupRamInfo[22].hash = "7287F537358716BB833BD1E04BDD8A4D"; //Shining Force Gaiden 2
    backupRamInfo[22].sramSize = 32 * 1024;
    backupRamInfo[23].hash = "ABDEF9E89D2963C70C5573D37EDD9D9D"; //Shining Force Gaiden - Final Conflict
    backupRamInfo[23].sramSize = 32 * 1024;

    //game gear games which run in sms mode
    smsModeInfo[0].hash = "290056BCB4303D3C53EA7B6AA2A268A7";
    smsModeInfo[1].hash = "BD9D0E51A16D2DFE8AC4D622F4882C08";
    smsModeInfo[2].hash = "16B6EA96908C17D4389A5907710A0F4E";
    smsModeInfo[3].hash = "7AD445DF21C499637E4E78AB1FFA1AB7";
    smsModeInfo[4].hash = "3CD1A4C27E330BBA7703EE22AC83B856";
    smsModeInfo[5].hash = "156CC252F0944114945C3B3F641A747A";
    smsModeInfo[6].hash = "D85DD6E38AC8AB3AF8123F74F12BF961";
    smsModeInfo[7].hash = "66A8E3133A047FA7D44968EFD3C30720"; //Fantastic Dizzy
    smsModeInfo[8].hash = "59E311B86223273016A1E70B9C881AF2";
    smsModeInfo[9].hash = "CBB734E03D22A48C2B5E071E0AEDD6EE";
    smsModeInfo[10].hash = "D52C5DB48882C0C126572A0CDEB5F472";
    smsModeInfo[11].hash = "2A3BDD1A6C35EEEDBF1A794ABFB57B87";
    smsModeInfo[12].hash = "6BB95912083C1BB48154BFE4EBF041BF";
    smsModeInfo[13].hash = "5F731E8F169F6FF914B94C4EAC2E6717";
    smsModeInfo[14].hash = "B90EB0B6F572B274E9AA94124CE92FD2";
    smsModeInfo[15].hash = "886649DC63ACA898CE30A32348192CD5";
    smsModeInfo[16].hash = "197E31DCAFEA7023CC948FEA29C50230";
    smsModeInfo[17].hash = "0D636C8D86DAFD4F2C337E34E94B1E41";
    smsModeInfo[18].hash = "FF832F0DDA876898AEBD3DE0ADF3D9B9";
    smsModeInfo[19].hash = "1E51D34E173EF3202EC33BB38C633EEB";
    smsModeInfo[20].hash = "FD82AF26EBBED24F57C4EEA8EDDF3136";
    smsModeInfo[21].hash = "D087B25D96F3F3E9338B0B5EC4FC2AA5";
    smsModeInfo[22].hash = "065CDB5AE5DB0FEBB432B77DCDC9283A";
    smsModeInfo[23].hash = "91AB09B8B4D25A08DD0EBE13003E54B5";
    smsModeInfo[24].hash = "93E08B96E19EB89C6BA7E2BF3824C990";
    smsModeInfo[25].hash = "613376B7B53E43BA17AE1B62DA3C9251"; //Excellent Dizzy Collection, The (E) [S][!]
    smsModeInfo[26].hash = "C68B86706784801EFF53A4CA4500FF21"; //Excellent Dizzy Collection, The (Prototype) [S][!]
    smsModeInfo[27].hash = "3AAB83A641BF3A26D68ED44F49C28714"; //Super Tetris
    smsModeInfo[28].hash = "D300CF467FCCD0F3E75DA39D9A5231BE"; //Super Tetris
    smsModeInfo[29].hash = "9B95B6E6609DAA8EA413F223F426C8FF"; //jang pung 2

    //games with additional on cart wram
    cartWramInfo[0].hash = "D047C10D6B27CF34CCB66340A089523A"; //The Castle
    cartWramInfo[0].size = 8 * 1024;
    cartWramInfo[0].start = 0x80;
    cartWramInfo[0].end = 0xbf;
    cartWramInfo[1].hash = "3F429704761D34D7793E7123A183D7F6"; //Othello
    cartWramInfo[1].size = 2 * 1024;
    cartWramInfo[1].start = 0x80;
    cartWramInfo[1].end = 0xbf;
    cartWramInfo[2].hash = "E97C20B86EA73248CC7AED602D46C3A4"; //Ernie Els Golf
    cartWramInfo[2].size = 8 * 1024;
    cartWramInfo[3].hash = "FF28E684EB3952AE3F7CF3E3D1C507EF"; //Kings Valley
    cartWramInfo[3].size = 8 * 1024;
    cartWramInfo[3].start = 0x20;
    cartWramInfo[3].end = 0x3f;

    //SG games with 8k ram extension
    sgWram8K[0].hash = "C12474E0A2556ADB2478BC4D99FABA51"; //Magical Kid Wiz

    //games which depends on uninitialized memory pattern
    wramPattern[0].hash = "752ED6CC6A06185A443FB1F84ACF8B6A"; //Alibaba
    wramPattern[0].value = 0xf0;
    wramPattern[1].hash = "31957C960E103BD95982405093B49CE8"; //Block Hole
    wramPattern[1].value = 0xf0;

    //games which don't pass the auto detection or have custom mapper
    customMapper[0].hash = "21B563031BB035F117D15DC53F406C2D"; //Xyzolog (KR)
    customMapper[0].mapper = NO_BANK_SWITCHING;
    customMapper[1].hash = "4EA64E3B2AD57EF7531890583C51DE30"; //4 PAK All Action (AU)
    customMapper[1].mapper = CUSTOM_4_PAK;
    customMapper[2].hash = "1F9B5AE214384C15E43E1D8EEE00C31F"; //Nemesis
    customMapper[2].mapper = KOREAN_8K_NEMESIS;
    customMapper[3].hash = "93BB1E1EE14AC53ADB9E6B9E0C135043"; //Trans-bot
    customMapper[3].mapper = STANDARD;
    customMapper[4].hash = "47DED00D11E42F3150BC2C660FBA2342"; //Trans-bot
    customMapper[4].mapper = STANDARD;
    customMapper[5].hash = "1AC740FA93B4A5DC019651AFF8498D9C"; //Trans-bot
    customMapper[5].mapper = STANDARD;
    customMapper[6].hash = "8D8979364BA71DA8268B2ABF288F7EDA"; //Trans-bot
    customMapper[6].mapper = STANDARD;
    customMapper[7].hash = "2A6EE78E2617886FE540DCB8A1500E90"; //Spy vs Spy
    customMapper[7].mapper = STANDARD;
    customMapper[8].hash = "1A97C84DADD3E93FE922D8F5F366625E"; //Spy vs Spy
    customMapper[8].mapper = STANDARD;
    customMapper[9].hash = "1A3B00C5B7382CFDE710EAC482E67C81"; //Spy vs Spy
    customMapper[9].mapper = STANDARD;
    customMapper[10].hash = "C6ADC5DB40D0DB75C9C167E4BB2ED918"; //Spy vs Spy
    customMapper[10].mapper = STANDARD;
    customMapper[11].hash = "FF28E684EB3952AE3F7CF3E3D1C507EF"; //kings valley
    customMapper[11].mapper = NO_BANK_SWITCHING;
    customMapper[12].hash = "2DB7AAABCA7F62D69DF466797C0D63D9"; //Janggun-ui adeul
    customMapper[12].mapper = CUSTOM_JANGGUN;
    customMapper[13].hash = "2C709BA8CDC3A1B7ADC5BC838EABF6F3"; //chapolim x dracula
    customMapper[13].mapper = STANDARD;
    customMapper[14].hash = "17FA28FEA6102A81AAFE7961AFF200DA"; //firetrack
    customMapper[14].mapper = STANDARD;
    customMapper[15].hash = "9B95B6E6609DAA8EA413F223F426C8FF"; //jang pung 2
    customMapper[15].mapper = CODEMASTERS;
    customMapper[16].hash = "A7462D79844A12A92941174252C9CA6E"; //Astro Flash
    customMapper[16].mapper = STANDARD;
    customMapper[17].hash = "2A6B0744637BA7ADCC17BF072638EFD2"; //Astro Flash
    customMapper[17].mapper = STANDARD;
    customMapper[18].hash = "D37F86C678B2AD0018518EA7278DB24B"; //Fantasy Zone The maze
    customMapper[18].mapper = STANDARD;
    customMapper[19].hash = "354B9B8E457CF7E81109989E11DACA3B"; //Fantasy Zone The maze
    customMapper[19].mapper = STANDARD;

    //games which don't expect port 3e (korean sms don't have it?)
    disablePort3e[0].hash = "3AAB83A641BF3A26D68ED44F49C28714"; //Super Tetris
    disablePort3e[1].hash = "D300CF467FCCD0F3E75DA39D9A5231BE"; //Super Tetris
    disablePort3e[2].hash = "D087B25D96F3F3E9338B0B5EC4FC2AA5"; //R.C. Grand Prix
    disablePort3e[3].hash = "065CDB5AE5DB0FEBB432B77DCDC9283A"; //R.C. Grand Prix
    disablePort3e[4].hash = "A11F8C275C80FC397D11F776E4197966"; //Power Boggle

    mapperHint[0].mapper = STANDARD;
    mapperHint[1].mapper = CODEMASTERS;
    mapperHint[2].mapper = KOREAN;
    mapperHint[3].mapper = KOREAN_8K;
    mapperHint[4].mapper = NO_BANK_SWITCHING;

    sram = 0;
    cartWram = 0;
    unload();
}

void Cart::unload() {
    free_mem(sram);
    free_mem(cartWram);
    ggInSmsMode = false;
    rom = 0;
    romSize = sramSize = cartWramSize = 0;
    loaded = false;
    copyHeaderChecked = false;
    sgUseRamExtension = false;
}

void Cart::calcHash() {
    unsigned char buffer[16];
    char temp[3];
    HL_MD5_CTX ctx;
    MD5* md5 = new MD5();
    md5->MD5Init(&ctx);
    md5->MD5Update(&ctx, rom, romSize);
    md5->MD5Final(buffer,&ctx);
    delete md5;
    romHash = "";
    for(unsigned i = 0; i < 16; i++) {
        snprintf(temp, 3, "%02x", buffer[i]);
        romHash += temp;
    }
}

void Cart::toLower(string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

bool Cart::isBios() {
    for(unsigned i=0; i < biosInfoSize; i++) {
        toLower(biosInfo[i].hash);
        if (romHash.compare(biosInfo[i].hash) == 0) {
            region = biosInfo[i].region;
            return true;
        }
    }
    return false;
}

bool Cart::is3dGlasses() {
    for(unsigned i=0; i < glassesInfoSize; i++) {
        toLower(glassesInfo[i].hash);
        if (romHash.compare(glassesInfo[i].hash) == 0) {
            return true;
        }
    }
    return false;
}

bool Cart::isPort3eDisable() {
    for(unsigned i=0; i < disablePort3eSize; i++) {
        toLower(disablePort3e[i].hash);
        if (romHash.compare(disablePort3e[i].hash) == 0) {
            return true;
        }
    }
    return false;
}

unsigned Cart::getCartWramSize() {
    for(unsigned i=0; i < cartWramInfoSize; i++) {
        toLower(cartWramInfo[i].hash);
        if (romHash.compare(cartWramInfo[i].hash) == 0) {
            cartWramInfoPos = i;
            return cartWramInfo[i].size;
        }
    }
    return 0;
}

void Cart::getSgWramSize() {
    for(unsigned i=0; i < sgWram8KSize; i++) {
        toLower(sgWram8K[i].hash);
        if (romHash.compare(sgWram8K[i].hash) == 0) {
            sgUseRamExtension = true;
            return;
        }
    }
    sgUseRamExtension = false;
}

unsigned Cart::getWramPattern() {
    for(unsigned i=0; i < wramPatternSize; i++) {
        toLower(wramPattern[i].hash);
        if (romHash.compare(wramPattern[i].hash) == 0) {
            return wramPattern[i].value;
        }
    }
    return 0;
}

Cart::Mappers Cart::getCustomMapper() {
    for(unsigned i=0; i < customMapperSize; i++) {
        toLower(customMapper[i].hash);
        if (romHash.compare(customMapper[i].hash) == 0) {
            return customMapper[i].mapper;
        }
    }
    return NONE;
}

bool Cart::useCartWram() {
    cartWramSize = getCartWramSize();
    if (cartWramSize == 0) return false;

    cartWram = new u8[cartWramSize];
    memset(cartWram, 0, cartWramSize);
    return true;
}

bool Cart::checkForSmsMode() {
    for(unsigned i=0; i < smsModeInfoSize; i++) {
        toLower(smsModeInfo[i].hash);
        if (romHash.compare(smsModeInfo[i].hash) == 0) {
            return true;
        }
    }
    return false;
}

bool Cart::checkForPalRegion() {
    for(unsigned i=0; i < palRegionsSize; i++) {
        toLower(palRegions[i].hash);
        if (romHash.compare(palRegions[i].hash) == 0) {
            return true;
        }
    }
    return false;
}

unsigned Cart::getCustomSramSize() {
    for(unsigned i=0; i < backupRamInfoSize; i++) {
        toLower(backupRamInfo[i].hash);
        if (romHash.compare(backupRamInfo[i].hash) == 0) {
            return backupRamInfo[i].sramSize;
        }
    }
    return 0;
}

void Cart::resetHints() {
    for (unsigned i = 0; i < mapperHintSize; i++) {
        mapperHint[i].hint = 0;
    }
}

void Cart::hintMapper(Mappers _mapper, unsigned add) {
    for (unsigned i = 0; i < mapperHintSize; i++) {
        if (mapperHint[i].mapper == _mapper) {
            mapperHint[i].hint += add;
            return;
        }
    }
}

unsigned Cart::getMapperHint(Mappers _mapper) {
    for (unsigned i = 0; i < mapperHintSize; i++) {
        if (mapperHint[i].mapper == _mapper) {
            return mapperHint[i].hint;
        }
    }
    return 0;
}

bool Cart::isAnyMapperHinted(unsigned limit) {
    for (unsigned i = 0; i < mapperHintSize; i++) {
        if (mapperHint[i].hint > limit) return true;
    }
    return false;
}

void Cart::detectMapper() {
    resetHints();
    for (unsigned i = 0; i < _min(0x8000, romSize); i++) {
        if (rom[i] == 0x32) { //LD (xxxx), A
            u16 addr = (unsigned)(rom[i+2] << 8) | rom[i+1];
            if (addr == 0xffff) {
                i += 2; hintMapper(STANDARD);
            } else if (addr == 0x8000 || addr == 0x4000) {
                i += 2; hintMapper(CODEMASTERS);
            } else if (addr == 0xa000) {
                i += 2; hintMapper(KOREAN);
            } else if ( (addr == 0x0001) || (addr == 0x0002) || (addr == 0x0003) ) {
                i += 2; hintMapper(KOREAN_8K, 2);
            }
        }
    }
    if (!isAnyMapperHinted(2)) {
        if (romSize <= (48 * 1024)) hintMapper(NO_BANK_SWITCHING, 4);
    }
    std::sort(mapperHint, mapperHint + mapperHintSize);
}

void Cart::setMapper() {
    for (unsigned i = 0; i < mapperHintSize; i++) {
        if ((mapperHint[i].hint > (getMapperHint(STANDARD) + 2))
        || ((mapperHint[i].hint > 0) && (getMapperHint(STANDARD) == 0))) {
            mapper = mapperHint[i].mapper;
            return;
        }
    }
}

void Cart::setRegion() {

    switch ( mapper ) {
        case BIOS:
            break;
        case KOREAN:
        case KOREAN_8K:
            region = JAPAN;
            break;
        case CODEMASTERS:
        case STANDARD:
        case NO_BANK_SWITCHING:
        default:
            if( checkForPalRegion() ) {
                region = EUROPE;
                break;
            }
            removeNonRomHeader();
            if (!detectHeader()) {
                region = JAPAN;
            } else {
                region = USA;
                u8 dataRegion = rom[0x7fff] >> 4;
                if (sys->getUnit() == _System::SMS) {
                    if (dataRegion == 3) {
                       region = JAPAN;
                    }
                } else if (sys->getUnit() == _System::GG) {
                    if (dataRegion == 5) {
                       region = JAPAN;
                    }
                }
            }
            break;
    }
}

bool Cart::detectHeader() {
    if (romSize < (32 * 1024)) {
        return false;
    }
    char buffer[8];
    memcpy(&buffer, &rom[0x7ff0], 8);
    string Sega = buffer;
    Sega = Sega.substr(0,8);
    if (Sega != "TMR SEGA") {
        return false;
    }
    return true;
}

void Cart::setSram() {
    sramSize = 0;
    if (sramSize = getCustomSramSize());
    else if (sys->getUnit() == _System::SMS || sys->getUnit() == _System::GG)
        sramSize = 8 * 1024;

    if (sramSize == 0) return;
    sram = new u8[sramSize];
    memset(sram, sramSize == 128 ? 0xff : 0, sramSize);
}

unsigned Cart::getSramSize() {
    return sramSize;
}

u8* Cart::getSram() {
    if (getSramSize() == 0) return 0;
    return sram;
}

void Cart::removeNonRomHeader() {
    if (copyHeaderChecked) {
        return;
    }
    if((romSize & 0x7fff) == 0x0200) { //remove 512 Byte header
        memmove(rom, rom + 512, romSize -= 512);
    }
    copyHeaderChecked = true;
}

void Cart::load(u8* _rom, u32 _romSize, bool _ggInSmsMode) {
    unload();
    ggInSmsMode = _ggInSmsMode;

    rom = _rom;
    romSize = _romSize;
    calcHash();

    cartDontExpectPort3e = isPort3eDisable();
    setSram();
    mapper = STANDARD;

    if (sys->getUnit() == _System::GG && !ggInSmsMode) {
        ggInSmsMode = checkForSmsMode();
    }

    sys->setGlassesRom( sys->getUnit() == _System::SMS ? is3dGlasses() : false );

    if (sys->getUnit() == _System::SG1000) {
        mapper = NO_BANK_SWITCHING;
        region = JAPAN;
        getSgWramSize();
    } else if (isBios()) {
        mapper = BIOS;
    } else {
        Mappers _custom = getCustomMapper();
        if (_custom != NONE) {
            mapper = _custom;
        } else {
            detectMapper();
            setMapper();
        }
        setRegion();
    }

    removeNonRomHeader();
    sys->setRegionFromCart( (_System::Region) region);
    sys->setSmsMode( (sys->getUnit() == _System::SMS) || ( (sys->getUnit() == _System::GG) && ggInSmsMode ) );
    sys->setGGMode( (sys->getUnit() == _System::GG) && !ggInSmsMode );
    sys->mapCartData(rom, romSize, sram, sramSize);
    if (useCartWram()) {
        sys->mapAddCartWram(cartWram, cartWramSize);
    }
    loaded = true;
}

