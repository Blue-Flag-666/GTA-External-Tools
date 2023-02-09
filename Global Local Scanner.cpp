#include "pch.hpp"
#include "Global Local Scanner.hpp"

namespace BF::Global_Local_Scanner
{
	void show(bool& show, bool& first)
	{
		ImGui::Begin("Global/Local Scanner", &show, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
		{
			if (first)
			{
				ImGui::SetWindowSize(ImVec2(570, 0));
				first = false;
			}
			static std::vector <ScanItem> list_addr;
			static auto                   begin          = 1,   end         = 5000000;
			static float                  scanner_height = 220, list_height = 200;

			if (static float last_height = 420; !equal(last_height, ImGui::GetContentRegionAvail().y))
			{
				const float height = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y;
				scanner_height     = max(220, height/(last_height-ImGui::GetStyle().ItemSpacing.y)*scanner_height);
				list_height        = max(200, height - scanner_height);
				scanner_height     = max(220, height - list_height);
				last_height        = ImGui::GetContentRegionAvail().y;
			}

			ImGui::Splitter(false, 3, &scanner_height, &list_height, 220, 200);

			ImGui::BeginChild("Scanner", ImVec2(0, scanner_height), false, ImGuiWindowFlags_AlwaysAutoResize);
			{
				static std::set <ScanResult>        scan_addr;
				static std::set <const ScanResult*> selection;
				static float                        option_width = 270, result_width = 270;

				if (static float last_width = 540; !equal(last_width, ImGui::GetContentRegionAvail().x))
				{
					const float width = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x;
					option_width      = max(270, width/(last_width - ImGui::GetStyle().ItemSpacing.x)*option_width);
					result_width      = max(270, width-option_width);
					option_width      = max(270, width-result_width);
					last_width        = ImGui::GetContentRegionAvail().x;
				}

				ImGui::Splitter(true, 3, &option_width, &result_width, 270, 270);

				ImGui::BeginChild("Scan Option", ImVec2(option_width, 0));
				{
					static ScanValue value;
					static int       scan_type = SCAN_INT;
					if (ImGui::Button("First Scan"))
					{
						scan_addr.clear();
						ScanGlobalLocal(scan_addr, scan_type, value);
					}
					ImGui::SameLine();
					if (ImGui::Button("Next Scan"))
					{
						ScanGlobalLocal(scan_addr, scan_type, value);
					}
					ImGui::SameLine();
					ImGui::Spacing(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(std::format("Count: {}", scan_addr.size()).c_str()).x - ImGui::GetStyle().ItemSpacing.x - 3 - 1, 0);
					ImGui::SameLine();
					ImGui::Text("Count: %lld", scan_addr.size());

					{
						const char* items[]  = { "Bool", "Number", "String" };
						static auto cur_item = 1;
						ImGui::Combo("Scan", &cur_item, items,IM_ARRAYSIZE(items));

						switch (cur_item)
						{
							case 0:
							{
								scan_type = SCAN_BOOL;
								break;
							}
							case 1:
							{
								if (scan_type & SCAN_NUMBER)
								{
									bool scan_float = scan_type & SCAN_FLOAT, scan_int = scan_type & SCAN_INT, scan_uint = scan_type & SCAN_UINT;
									ImGui::Checkbox("Float", &scan_float);
									ImGui::SameLine();
									ImGui::Checkbox("Int", &scan_int);
									ImGui::SameLine();
									ImGui::Checkbox("UInt", &scan_uint);
									if (const auto new_scan_type = (scan_float ? SCAN_FLOAT : SCAN_NONE) | (scan_int ? SCAN_INT : SCAN_NONE) | (scan_uint ? SCAN_UINT : SCAN_NONE))
									{
										scan_type = new_scan_type;
									}
								}
								else
								{
									scan_type = SCAN_INT;
								}
								break;
							}
							case 2:
							{
								scan_type = SCAN_STRING;
								break;
							}
							default: ;
						}
					}

					if (scan_type & SCAN_BOOL)
					{
						value          = ScanValue(value.bool_value);
						int bool_value = value.bool_value;
						ImGui::RadioButton("True", &bool_value, true);
						ImGui::SameLine();
						ImGui::RadioButton("False", &bool_value, false);
						value.bool_value = bool_value;
					}
					else if (scan_type & SCAN_STRING)
					{
						value.reset(value.string_value);
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Value").x - ImGui::CalcTextSize(std::format("Length: {}", strlen(value.string_value)).c_str()).x - ImGui::GetStyle().ItemSpacing.x * 2 - 3);
						ImGui::InputText("Value", value.string_value, MAX_STR_LEN);
						ImGui::SameLine();
						ImGui::Text("Length: %lld", strlen(value.string_value));
						if (strlen(value.string_value) == static_cast <size_t>(MAX_STR_LEN - 1))
						{
							ImGui::Text("Warning: Max string length reached. (%d)", MAX_STR_LEN - 1);
						}
					}
					else
					{
						if (scan_type & SCAN_FLOAT)
						{
							auto x = static_cast <double>(value.float_value);
							ImGui::InputDouble("Value", &x);
							value = ScanValue(x);
						}
						else if (scan_type & SCAN_UINT)
						{
							uint32_t x = value.uint_value;
							ImGui::InputScalar("Value", ImGuiDataType_U32, &x);
							value = ScanValue(x);
						}
						else
						{
							int x = value.int_value;
							ImGui::InputInt("Value", &x, 0);
							value = ScanValue(x);
						}
					}

					ImGui::Separator();
					ImGui::Text("Scan Options");
					ImGui::InputInt("Begin", &begin, 0);
					ImGui::InputInt("End", &end, 0);

					ImGui::SetCursorScreenPos(ImGui::GetContentRegionMaxAbs() - ImVec2(ImGui::CalcTextSize("Add").x + ImGui::GetStyle().ItemSpacing.x + 3, ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2 + 3));
					if (ImGui::Button("Add"))
					{
						for (const auto x : selection)
						{
							list_addr.emplace_back(x->offset, x->type);
						}
					}
				}
				ImGui::EndChild();

				ImGui::SameLine();
				ImGui::BeginChild("Scan Result", ImVec2(result_width, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y));
				{
					/*if (scan_addr.empty())//debug
					{
						for (auto i = 1; i <= 100; i++)
						{
							scan_addr.insert({ i * 8, SCAN_BOOL });
						}
					}*/

					if (ImGui::BeginTable("Result Table", 4, (result_width < 320 ? ImGuiTableFlags_SizingFixedFit : ImGuiTableFlags_None) | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_ScrollY))
					{
						ImGui::TableSetupColumn("Offset");
						ImGui::TableSetupColumn("Address");
						ImGui::TableSetupColumn("Value");
						ImGui::TableSetupColumn("Previous");
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableHeadersRow();
						{
							for (auto item = scan_addr.begin(); item != scan_addr.end(); ++item)
							{
								const bool selected = selection.contains(&*item);

								ImGui::TableNextRow();

								if (ImGui::TableSetColumnIndex(0))
								{
									if (ImGui::Selectable(std::to_string(item->offset).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick))
									{
										static auto last_click = &*item;
										if (ImGui::GetIO().KeyCtrl)
										{
											if (selected)
											{
												selection.erase(&*item);
											}
											else
											{
												selection.insert(&*item);
											}
										}
										else if (ImGui::GetIO().KeyShift)
										{
											if (last_click != &*item)
											{
												auto last = scan_addr.find(*last_click);
												auto left = &last, right = &item;
												if (**right < **left)
												{
													swap(left, right);
												}
												for_each(*left, *right, [](const auto& x)
												{
													selection.insert(&x);
												});
												selection.insert(&**right);
											}
										}
										else
										{
											selection.clear();
											selection.insert(&*item);
										}
										last_click = &*item;

										if (ImGui::IsMouseDoubleClicked(0))
										{
											for (const auto x : selection)
											{
												list_addr.emplace_back(x->offset, x->type);
											}
											selection.clear();
										}
									}

									if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
									{
										for (const auto x : selection)
										{
											scan_addr.erase(*x);
										}
										selection.clear();
										break;
									}
								}

								if (ImGui::TableSetColumnIndex(1))
								{
									ImGui::Text("%08llX", GTA5.get_global_addr(item->offset));
								}

								if (ImGui::TableSetColumnIndex(2))
								{
									switch (item->type)
									{
										case SCAN_BOOL:
										{
											ImGui::Text(GTA5.get_global <bool>(item->offset) ? "True" : "False");
											break;
										}
										case SCAN_FLOAT:
										{
											ImGui::Text("%f", static_cast <double>(GTA5.get_global <float>(item->offset)));
											break;
										}
										case SCAN_INT:
										{
											ImGui::Text("%d", GTA5.get_global <int>(item->offset));
											break;
										}
										case SCAN_UINT:
										{
											ImGui::Text("%u", GTA5.get_global <uint32_t>(item->offset));
											break;
										}
										case SCAN_STRING:
										{
											ImGui::Text("%s", GTA5.get_global <const char*>(item->offset));
											break;
										}
										case SCAN_NUMBER:
										case SCAN_NONE: ;
									}
								}

								if (ImGui::TableSetColumnIndex(3))
								{
									switch (item->type)
									{
										case SCAN_BOOL:
										{
											ImGui::Text(item->prev.bool_value ? "True" : "False");
											break;
										}
										case SCAN_FLOAT:
										{
											ImGui::Text("%f", static_cast <double>(item->prev.float_value));
											break;
										}
										case SCAN_INT:
										{
											ImGui::Text("%d", item->prev.int_value);
											break;
										}
										case SCAN_UINT:
										{
											ImGui::Text("%u", item->prev.uint_value);
											break;
										}
										case SCAN_STRING:
										{
											ImGui::Text("%s", item->prev.string_value);
											break;
										}
										case SCAN_NUMBER:
										case SCAN_NONE: ;
									}
								}
							}
						}
						ImGui::EndTable();
					}
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::Spacing();
			ImGui::BeginChild("Address List", ImVec2(0, list_height));
			{
				/*if (list_addr.empty())//debug
				{
					for (auto i = 1; i <= 5; i++)
					{
						list_addr.emplace_back(i * 8, SCAN_BOOL);
					}
				}*/

				if (ImGui::BeginTable("Address List", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_ScrollY))
				{
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Offset");
					ImGui::TableSetupColumn("Address");
					ImGui::TableSetupColumn("Type");
					ImGui::TableSetupColumn("Value");
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableHeadersRow();
					{
						static std::set <size_t> selection;
						for (size_t i = 0; i < list_addr.size(); i++)
						{
							const auto item     = list_addr.begin() + static_cast <ptrdiff_t>(i);
							const bool selected = selection.contains(i);

							ImGui::TableNextRow();

							if (ImGui::TableSetColumnIndex(0))
							{
								static auto popup_opened = static_cast <size_t>(-1);
								if (ImGui::Selectable(std::format("{}##{}", item->name, i).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick))
								{
									static auto last_click = i;
									if (ImGui::GetIO().KeyCtrl)
									{
										if (selected)
										{
											selection.erase(i);
										}
										else
										{
											selection.insert(i);
										}
									}
									else if (ImGui::GetIO().KeyShift)
									{
										if (last_click != i)
										{
											auto left = &last_click, right = &i;
											if (*right < *left)
											{
												std::swap(left, right);
											}
											for (auto j = *left; j <= *right; j++)
											{
												selection.insert(j);
											}
										}
									}
									else
									{
										selection.clear();
										selection.insert(i);
									}
									last_click = i;

									if (ImGui::IsMouseDoubleClicked(0))
									{
										popup_opened = i;
									}
								}

								if (bool opened = popup_opened == i; opened)
								{
									const auto window = ImGui::GetCurrentWindow()->ParentWindow->ParentWindow;
									ImGui::SetNextWindowPos(window->Pos + window->Size / 2, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
									if (ImGui::Begin(std::format("Edit Item##", i).c_str(), &opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
									{
										static ScanItem  unsaved = *item;
										static ScanValue unsaved_value;
										static auto      new_unsaved = true;
										if (new_unsaved)
										{
											unsaved = *item;
											switch (item->type)
											{
												case SCAN_BOOL:
												{
													unsaved_value = ScanValue(GTA5.get_global <bool>(item->offset));
													break;
												}
												case SCAN_FLOAT:
												{
													unsaved_value = ScanValue(GTA5.get_global <float>(item->offset));
													break;
												}
												case SCAN_INT:
												{
													unsaved_value = ScanValue(GTA5.get_global <int>(item->offset));
													break;
												}
												case SCAN_UINT:
												{
													unsaved_value = ScanValue(GTA5.get_global <uint32_t>(item->offset));
													break;
												}
												case SCAN_STRING:
												{
													unsaved_value = ScanValue(GTA5.get_global <const char*>(item->offset));
													break;
												}
												case SCAN_NUMBER:
												case SCAN_NONE: ;
											}
											new_unsaved = false;
										}

										ImGui::Text("Name");
										{
											ImGui::InputText("Name", unsaved.name, MAX_NAME_LEN);
											ImGui::SameLine();
											ImGui::Text("Length: %lld", std::strlen(unsaved.name));
											if (std::strlen(unsaved.name) == MAX_NAME_LEN - 1)
											{
												ImGui::Text("Warning: Max name length reached. (%d)", MAX_NAME_LEN - 1);
											}
										}

										ImGui::Text("type");
										{
											const char* items[]  = { "Bool", "Float", "Int", "UInt", "String" };
											auto        cur_item = -1;
											for (int x = unsaved.type; x; cur_item++)
											{
												x = x >> 1;
											}
											ImGui::Combo("type", &cur_item, items,IM_ARRAYSIZE(items));
											unsaved.type = static_cast <ScanType>(1 << cur_item);
										}

										ImGui::Text("Value");
										{
											switch (unsaved.type)
											{
												case SCAN_BOOL:
												{
													int bool_value = unsaved_value.bool_value;
													ImGui::RadioButton("True", &bool_value, true);
													ImGui::SameLine();
													ImGui::RadioButton("False", &bool_value, false);
													unsaved_value.bool_value = bool_value;
													break;
												}
												case SCAN_FLOAT:
												{
													auto x = static_cast <double>(unsaved_value.float_value);
													ImGui::InputDouble("Value", &x);
													unsaved_value = ScanValue(x);
													break;
												}
												case SCAN_INT:
												{
													int x = unsaved_value.int_value;
													ImGui::InputInt("Value", &x, 0);
													unsaved_value = ScanValue(x);
													break;
												}
												case SCAN_UINT:
												{
													uint32_t x = unsaved_value.uint_value;
													ImGui::InputScalar("Value", ImGuiDataType_U32, &x);
													unsaved_value = ScanValue(x);
													break;
												}
												case SCAN_STRING:
												{
													unsaved_value.reset(unsaved_value.string_value);
													ImGui::InputText("Value", unsaved_value.string_value, MAX_STR_LEN);
													ImGui::SameLine();
													ImGui::Text("Length: %lld", strlen(unsaved_value.string_value));
													if (strlen(unsaved_value.string_value) == static_cast <size_t>(MAX_STR_LEN - 1))
													{
														ImGui::Text("Warning: Max string length reached. (%d)", MAX_STR_LEN - 1);
													}
													break;
												}
												case SCAN_NUMBER:
												case SCAN_NONE: ;
											}
										}

										ImGui::SetItemDefaultFocus();
										if (ImGui::Button("OK", ImVec2(120, 0)))
										{
											*item = unsaved;
											switch (item->type)
											{
												case SCAN_BOOL:
												{
													GTA5.set_global <bool>(item->offset, unsaved_value.bool_value);
													break;
												}
												case SCAN_FLOAT:
												{
													GTA5.set_global <float>(item->offset, unsaved_value.float_value);
													break;
												}
												case SCAN_INT:
												{
													GTA5.set_global <int>(item->offset, unsaved_value.int_value);
													break;
												}
												case SCAN_UINT:
												{
													GTA5.set_global <uint32_t>(item->offset, unsaved_value.uint_value);
													break;
												}
												case SCAN_STRING:
												{
													strcpy_s(static_cast <char*>(int64_to_void(GTA5.get_global_addr(item->offset))), strlen(unsaved_value.string_value), unsaved_value.string_value);
													break;
												}
												case SCAN_NUMBER:
												case SCAN_NONE: ;
											}
											new_unsaved = true;
											opened      = false;
										}
										ImGui::SameLine();
										if (ImGui::Button("Cancel", ImVec2(120, 0)))
										{
											new_unsaved = true;
											opened      = false;
										}
										ImGui::End();
									}
									if (!opened)
									{
										popup_opened = static_cast <size_t>(-1);
									}
								}
								else
								{
									GImGui->NextWindowData.ClearFlags();

									if (popup_opened == static_cast <size_t>(-1) && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
									{
										for (const auto it : std::ranges::reverse_view(selection))
										{
											list_addr.erase(list_addr.begin() + static_cast <ptrdiff_t>(it));
										}
										selection.clear();
										break;
									}
								}
							}

							if (ImGui::TableSetColumnIndex(1))
							{
								ImGui::Text("%d", item->offset);
							}

							if (ImGui::TableSetColumnIndex(2))
							{
								ImGui::Text("%08llX", GTA5.get_global_addr(item->offset));
							}

							if (ImGui::TableSetColumnIndex(3))
							{
								ImGui::Text("%s", to_string(item->type).c_str());
							}

							if (ImGui::TableSetColumnIndex(4))
							{
								switch (item->type)
								{
									case SCAN_BOOL:
									{
										ImGui::Text(GTA5.get_global <bool>(item->offset) ? "True" : "False");
										break;
									}
									case SCAN_FLOAT:
									{
										ImGui::Text("%f", static_cast <double>(GTA5.get_global <float>(item->offset)));
										break;
									}
									case SCAN_INT:
									{
										ImGui::Text("%d", GTA5.get_global <int>(item->offset));
										break;
									}
									case SCAN_UINT:
									{
										ImGui::Text("%u", GTA5.get_global <uint32_t>(item->offset));
										break;
									}
									case SCAN_STRING:
									{
										ImGui::Text("%s", GTA5.get_global <const char*>(item->offset));
										break;
									}
									case SCAN_NUMBER:
									case SCAN_NONE: ;
								}
							}
						}
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void ScanGlobalLocal(std::set <ScanResult>& s, const int type, const ScanValue& value)
	{
		if (type & SCAN_BOOL)
		{
			ScanGlobalLocal <bool, SCAN_BOOL>(s, value.bool_value);
		}
		else if (type & SCAN_STRING)
		{
			ScanGlobalLocal <const char*, SCAN_STRING>(s, value.string_value);
		}
		else
		{
			if (type & SCAN_FLOAT)
			{
				ScanGlobalLocal <float, SCAN_FLOAT>(s, value.float_value);
			}
			if (type & SCAN_INT)
			{
				ScanGlobalLocal <int, SCAN_INT>(s, value.int_value);
			}
			if (type & SCAN_UINT)
			{
				ScanGlobalLocal <uint32_t, SCAN_UINT>(s, value.uint_value);
			}
		}
	}
}
