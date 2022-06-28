
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"

#include <sstream>
#include "utils.h"
#include "io.h"
#include "clibretro.h"

#ifndef _WIN32
#define stricmp strcasecmp
#endif

ImGuiFileDialog romloader;
const char *ss_filters = "Savestates (*.state){.state}";
static ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_None;

enum
{
  digi_pad,
  ana_pad,
  mousie
};
int controller_type = digi_pad;

const char *checkbox_allowable[] = {"enabled|disabled", "disabled|enabled", "True|False", "False|True", "On|Off", "Off|On"};
const char *true_vals[] = {"enabled", "true", "on"};

static bool coreselect = false;
static std::string filenamepath;


static auto vector_getter = [](void *data, int n, const char **out_text)
{
  const std::vector<core_info> *v = (std::vector<core_info> *)data;
  *out_text = v->at(n).core_name.c_str();
  return true;
};

bool loadfile(CLibretro *instance, const char *file, const char* core_file,bool pergame)
{
  int hits = 0;
  int selected_core = 0;
  if(core_file != NULL)
  {
    instance->core_load((char *)file, pergame, (char*)core_file);
    return false;
  }
  for (size_t i = 0; i < instance->cores.size(); i++)
  {
    auto &core = instance->cores.at(i);
    std::string core_ext = core.core_extensions;
    std::string ext = file;
    ext = ext.substr(ext.find_last_of(".") + 1);
    if (core_ext.find(ext) != std::string::npos)
    {
      hits++;
      selected_core = i;
    }
  }
  if (hits == 1)
  {
    instance->core_load((char *)file, pergame, (char *)instance->cores.at(selected_core).core_path.c_str());
    return false;
  }
  else
  {
    filenamepath = file;
    coreselect = true;
    return true;
  }
}


void popup_widget(bool *flag, const char* title, const char *msg)
{
  ImGui::OpenPopup(title);
       if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Text(msg);
                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120,0))) { ImGui::CloseCurrentPopup();  *flag = false;}
                ImGui::EndPopup();
            }
}

void sdlggerat_menu(CLibretro *instance, std::string *window_str, int *selected_in, bool *isselected_inp)
{

static bool inputsettings = false;
static bool coresettings = false;
static bool aboutbox = false;


  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Load ROM/ISO"))
        romloader.OpenModal("ChooseFileDlgKey", " Choose a ROM/ISO", instance->coreexts.c_str(), ".", "", 1, nullptr, flags);

      ImGui::Separator();

      if (ImGui::MenuItem("Load Savestate"))
      {
        if (instance->core_isrunning())
          romloader.OpenModal("LoadSaveState", "Load a savestate", ss_filters, ".", "", 1, nullptr, flags);
      }

      if (ImGui::MenuItem("Save Savestate"))
      {
        if (instance->core_isrunning())
          romloader.OpenModal("SaveSaveState", "Save a savestate", ss_filters, ".", "", 1, IGFDUserDatas("SaveFile"), ImGuiFileDialogFlags_ConfirmOverwrite);
      }

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Options"))
    {
      if (ImGui::MenuItem("Core Settings..."))
        coresettings = true;
      if (ImGui::MenuItem("Input Settings..."))
        inputsettings = true;

      ImGui::Separator();
      if (ImGui::BeginMenu("Input device"))
      {
        if (ImGui::BeginMenu("Joypad"))
        {
          if (ImGui::MenuItem("Analog", nullptr, controller_type == ana_pad))
            controller_type = ana_pad;
          if (ImGui::MenuItem("Digital", nullptr, controller_type == digi_pad))
            controller_type = digi_pad;
          ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Mouse", nullptr, controller_type == mousie))
          controller_type = mousie;
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }

    if (ImGui::MenuItem("About..."))
      aboutbox = true;

    ImGui::EndMainMenuBar();
  }

  if (romloader.Display("ChooseFileDlgKey", 0, ImVec2(550, 400)))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      coreselect = loadfile(instance, (char *)filePathName.c_str(),NULL,false);
      filenamepath = filePathName;
    }

    romloader.Close();
  }

  if (coreselect)
  {
     std::vector<core_info> cores_info;
      cores_info.clear();
      bool found = false;
      for (auto &core : instance->cores)
      {
        std::string core_ext = core.core_extensions;
        std::string ext = filenamepath;
        ext = ext.substr(ext.find_last_of(".") + 1);
        if (core_ext.find(ext) != std::string::npos)
        {
          cores_info.push_back(core);
          found=true;
        }
      }
    if(!found)
    {

      popup_widget(&coreselect, "Core Load Error", "There is no core to load this particular bit of content.");
      return;
    }
    else
    ImGui::OpenPopup("Select a core");
    if (ImGui::BeginPopupModal("Select a core", &coreselect, ImGuiWindowFlags_AlwaysAutoResize))
    {
     
      static int listbox_item_current = 0;
      ImGui::PushItemWidth(200);
      ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
      ImGui::ListBox("Select a core",
                     &listbox_item_current, vector_getter, static_cast<void *>(&cores_info), cores_info.size());
      if (ImGui::Button("OK"))
      {
        instance->core_load((char *)filenamepath.c_str(), false, (char *)cores_info.at(listbox_item_current).core_path.c_str());
        coreselect = false;
      }
      ImGui::BulletText("WTFwegerrat couldn't determine the core to use.");
      ImGui::BulletText("Choose the specific core to load the ROM/ISO wanted.");
      ImGui::EndPopup();
    }
  }

  if (romloader.Display("LoadSaveState", 0, ImVec2(550, 400)))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      instance->core_savestate(filePathName.c_str(), false);
      // action
    }
    // close
    romloader.Close();
  }

  if (romloader.Display("SaveSaveState", 0, ImVec2(550, 400)))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      instance->core_savestate(filePathName.c_str(), true);
    }
    // close
    romloader.Close();
  }

  if (inputsettings && instance->core_isrunning())
  {
    ImGui::PushItemWidth(200);
    ImGui::SetNextWindowSize(ImVec2(550, 660), ImGuiCond_FirstUseEver);

    if(!instance->core_inputbinds.size())
    {
      popup_widget(&inputsettings, "No input settings", "There is no input settings for this particular core.");
      return;
    }


    ImGui::OpenPopup("Input Settings");

    if (ImGui::BeginPopupModal("Input Settings", &inputsettings,ImGuiWindowFlags_AlwaysAutoResize))
    {

      std::string str2;
      for (size_t i = 0; i < instance->core_inputbinds.size(); i++)
      {
        auto &bind = instance->core_inputbinds[i];
        if (bind.description == "")
          continue;

        int total_w = 300;
        ImGui::Text("%s", bind.description.c_str());
        std::string script = "##" + bind.description;
        char *button_str = (char *)bind.joykey_desc.c_str();
        ImGui::SameLine(350);
        ImGui::SetNextItemWidth(total_w);
        ImGui::InputText(script.c_str(), button_str, 0, 0, NULL);
        if (ImGui::IsItemClicked())
        {
          *selected_in = i;
          *isselected_inp = true;
        }
      }
      if (ImGui::Button("OK"))
      {
        inputsettings = false;
        *isselected_inp = false;
        save_inpcfg();
      }
      ImGui::EndPopup();
    }
  }

  if (aboutbox)
  {
    ImGui::PushItemWidth(200);
    ImGui::OpenPopup("About WTFgerrat");
    if (ImGui::BeginPopupModal("About WTFgerrat", &aboutbox, ImGuiWindowFlags_AlwaysAutoResize))
    {
      std::string date = "Built on " __DATE__ " at " __TIME__ " (GMT+10)\n\n";
      ImGui::Text("%s", date.c_str());
      ImGui::BulletText("WTFwegerrat is for personal use.");
      ImGui::BulletText("Support for all libretro cores is not expected.");
      ImGui::BulletText("Support/bug reports will be ignored.");
      std::string greetz =

          R"foo(

Greetz:

Higor Eurípedes
Andre Leiradella
Andrés Suárez
Brad Parker
Chris Snowhill
Hunter Kaller
Alfred Agrell
Lars Viklund
Samuel Neves
Peter Pawlowski
Gian-Carlo Pascutto
Chastity
Genju
)foo";
      ImGui::Text("%s", greetz.c_str());

      if (ImGui::Button("OK"))
      {
        aboutbox = false;
      }
      ImGui::EndPopup();
    }
  }

  if (coresettings && instance->core_isrunning())
  {
    ImGui::PushItemWidth(200);
    ImGui::SetNextWindowSize(ImVec2(550, 660), ImGuiCond_FirstUseEver);

    if(!instance->core_variables.size())
    {
      popup_widget(&coresettings, "No core settings", "There is no core settings for this particular core.");
      return;
    }


    ImGui::OpenPopup("Core Settings");

    if (ImGui::BeginPopupModal("Core Settings", &coresettings,ImGuiWindowFlags_AlwaysAutoResize))
    {
      for (auto &bind : instance->core_variables)
      {

        if (bind.config_visible)
        {
          std::string descript = bind.description;
          std::string usedv = bind.usevars;
          std::string var = bind.var;

          int sel_idx = bind.sel_idx;
          std::string current_item = bind.config_vals[sel_idx];
          bool checkbox_made = false;
          bool checkbox_enabled = false;
          for (int j = 0; j < IM_ARRAYSIZE(checkbox_allowable); j++)
          {
            if (checkbox_enabled)
              break;
            if (stricmp(usedv.c_str(), checkbox_allowable[j]) == 0)
            {
              checkbox_made = true;
              for (int k = 0; k < IM_ARRAYSIZE(true_vals); k++)
                if (stricmp(var.c_str(), true_vals[k]) == 0)
                  checkbox_enabled = true;
            }
          }

          std::string hidden = "##" + descript;
          if (checkbox_made)
          {

            int total_w = descript.length();
            ImGui::Text("%s", descript.c_str());
            ImGui::SameLine(650 - 200);
            ImGui::SetNextItemWidth(total_w);
            if (ImGui::Checkbox(hidden.c_str(), &checkbox_enabled))
            {
              bind.sel_idx ^= 1;
              std::string change = bind.config_vals[bind.sel_idx];
              bind.var = change;
              instance->variables_changed = true;
            }
          }
          else
          {
            int total_w = 200;
            ImGui::Text("%s", descript.c_str());
            ImGui::SameLine(650 - 200);
            ImGui::SetNextItemWidth(total_w);
            if (ImGui::BeginCombo(hidden.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
            {
              for (size_t n = 0; n < bind.config_vals.size(); n++)
              {
                bool is_selected = (bind.sel_idx ==n); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(bind.config_vals[n].c_str(), is_selected))
                {
                  bind.sel_idx = n;
                  std::string change = bind.config_vals[bind.sel_idx];
                  bind.var = change;
                  instance->variables_changed = true;
                }

                if (is_selected)
                  ImGui::SetItemDefaultFocus();
              }
              ImGui::EndCombo();
            }
          }
        }
      }

      // click ok when finished adjusting
      if (ImGui::Button("OK"))
      {
        coresettings = false;
        instance->save_coresettings();
      }

      ImGui::EndPopup();
    }
  }
}