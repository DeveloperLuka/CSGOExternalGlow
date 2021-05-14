#include <Windows.h>
#include <iostream>
#include "Mem.h"
#include "offsets.h"

//defines Mem class
Mem MemClass;

//defines hazedumper namespaces
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

//variable struct
struct variables
{
    uintptr_t localPlayer;
    uintptr_t gameModule;
    uintptr_t engineModule;
    uintptr_t glowObject;
} val;

//struct for the color and tean/enemy team variables
struct ClrRender
{
    BYTE red, green, blue;
};
ClrRender clrEnemy;
ClrRender clrTeam;

//all variables for writing memory for the glow 
struct GlowStruct
{
    BYTE base[4];
    float red;
    float green;
    float blue;
    float alpha;
    BYTE buffer[16];
    bool renderWhenOccluded;
    bool renderWhenUnOccluded;
    bool fullBloom;
    BYTE buffer1[5];
    int glowStyle;
};

GlowStruct SetGlowColor(GlowStruct Glow, uintptr_t entity)
{
    //reads rpm if an player is defusing and saves it in a bool
    bool defusing = MemClass.readMem<bool>(entity + m_bIsDefusing);
    //sets glow color to white if an player is defusing
    if (defusing)
    {
        Glow.red = 1.0f;
        Glow.green = 1.0f;
        Glow.blue = 1.0f;
    }
    else
    {
        int health = MemClass.readMem<int>(entity + m_iHealth);
        Glow.red = health * -0.01 + 1;
        Glow.green = health * 0.01;
    }
    Glow.alpha = 1.0f;
    Glow.renderWhenOccluded = true;
    Glow.renderWhenUnOccluded = true;
    return Glow;
}

//sets glow color to blue if its an teammate
void SetTeamGlow(uintptr_t entity, int glowIndex)
{
    GlowStruct TGlow;
    TGlow = MemClass.readMem<GlowStruct>(val.glowObject + (glowIndex * 0x38));

    TGlow.blue = 1.0f;
    TGlow.alpha = 1.0f;
    TGlow.renderWhenOccluded = true;
    TGlow.renderWhenUnOccluded = true;
    MemClass.writeMem<GlowStruct>(val.glowObject + (glowIndex * 0x38), TGlow);
}

//sets enemy glow color to its hp
void SetEnemyGlow(uintptr_t entity, int glowIndex)
{
    GlowStruct EGlow;
    EGlow = MemClass.readMem<GlowStruct>(val.glowObject + (glowIndex * 0x38));
    EGlow = SetGlowColor(EGlow, entity);
    MemClass.writeMem<GlowStruct>(val.glowObject + (glowIndex * 0x38), EGlow);
}

void HandleGlow()
{
    //rpm to get glowobject
    val.glowObject = MemClass.readMem<uintptr_t>(val.gameModule + dwGlowObjectManager);
    //saves you're own team by adding the localPlayer to the team
    int myTeam = MemClass.readMem<int>(val.localPlayer + m_iTeamNum);

    //loops through the entity list
    for (short int i = 0; i < 64; i++)
    {
        //saves the current entity to an uintptr_t
        uintptr_t entity = MemClass.readMem<uintptr_t>(val.gameModule + dwEntityList + i * 0x10);
        //checks if the current entity is an entity
        if (entity != NULL)
        {
            //saves the glowindex of the current entity
            int glowIndx = MemClass.readMem<int>(entity + m_iGlowIndex);
            //saves the team of the current entity
            int entityTeam = MemClass.readMem<int>(entity + m_iTeamNum);

            //checks if the team from the current entity is the same from the localPlayer
            if (myTeam == entityTeam)
            {
                MemClass.writeMem<ClrRender>(entity + m_clrRender, clrTeam);
                SetTeamGlow(entity, glowIndx);
            }
            else
            {
                MemClass.writeMem<ClrRender>(entity + m_clrRender, clrEnemy);
                SetEnemyGlow(entity, glowIndx);
            }
        }
    }
}

//sets brightness
void SetBrightness()
{
    clrTeam.red = 0;
    clrTeam.blue = 255;
    clrTeam.green = 0;

    clrEnemy.red = 255;
    clrEnemy.blue = 0;
    clrEnemy.green = 0;

    float brightness = 5.0f;

    int ptr = MemClass.readMem<int>(val.engineModule + model_ambient_min);
    int xorptr = *(int*)&brightness ^ ptr;
    MemClass.writeMem<int>(val.engineModule + model_ambient_min, xorptr);
}

int main()
{
    //gets process id
    int procID = MemClass.getProcess(L"csgo.exe");
    //gets the client.dll
    val.gameModule = MemClass.getModule(procID, L"client.dll");
    //gets the engine.dll
    val.engineModule = MemClass.getModule(procID, L"engine.dll");

    //gets the localPlayer
    val.localPlayer = MemClass.readMem<uintptr_t>(val.gameModule + dwLocalPlayer);

    //checks if the localPlayer is null and if it is it loops untili it finds the localPlayer
    if (val.localPlayer == NULL)
        while (val.localPlayer == NULL)
            val.localPlayer = MemClass.readMem<uintptr_t>(val.gameModule + dwLocalPlayer);

    //jumps to the brightness fumction
    SetBrightness();

    //infinite loop
    while (true)
    {
        //checks if the insert key is pressed
        if (GetKeyState(VK_INSERT) & 1)
            //jumps to the handleglow function
            HandleGlow();

        //sleeps for one millisecond that the programm doesn't run 1billion times per second and spams the ram
        Sleep(1);
    }
    //returns 0 if everything worked
    return 0;
}