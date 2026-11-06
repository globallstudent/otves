# Russian construction terminology & Russian UI-label guide (mobile drawing + estimating app for private builders in Uzbekistan and Russia)

Research date: 2026-11-06. Notes are in English; Russian terms are in Cyrillic; Uzbek words are in Uzbek Latin (with Uzbek Cyrillic where the source used it).

**Legend used in all tables**
- ✅ = confirmed by a source cited in "Cited Findings" of that section
- ◐ = partly confirmed (commercial listing, secondary article, or search snippet whose page was not opened)
- ✱ = NOT verified in this research: author's domain knowledge or a subjective recommendation. Treat as a hypothesis to check with real builders.
- Label length counts are Cyrillic characters, including spaces.

---

## 1. Drawing/editor tools: what Russian CAD/BIM and home-planning apps call each tool, and which variant is standard

### Takeaway
Russian BIM/CAD localizations agree on the **element nouns** (Стена, Окно, Дверь, Перекрытие, Колонна, Балка, Крыша, Лестница). Both ArchiCAD and Renga separate **Проём** (opening in a wall, door/window-like) from **Отверстие** (a cut through slabs, roofs or any element). **Edit commands** are not standardized: Revit RU help uses «Перенести» and «Зеркало», ArchiCAD RU uses «Зеркальное отражение», and mobile/OS conventions (Flutter Material RU) use «Копировать», «Вставить», «Выбрать все». **Storey** naming splits ArchiCAD «Этаж» vs Revit/Renga «Уровень» (the latter ✱). For a builder-facing mobile app, a consistent set of **short infinitive verbs** for actions plus **«Этаж»** for storeys is the safest choice (subjective).

### Cited Findings
- ArchiCAD 29 Russian help calls the stair tool «Инструмент Лестница». It refers to storeys as «Этаж»/«Этажи» ("Лестницы могут менять свою высоту при изменении высоты связанных с ними Этажей") and to views as «План Этажа» and «3D-окно». Stair parts are «Проступь» (tread) and «Подступенок» (riser), and curtain walls are «Навесные Стены». The help uses Title Case for these names — [ArchiCAD 29 RUS help, Stair tool](https://help.graphisoft.com/AC/29/RUS/_AC29_Help/040_ElementsVB/040_ElementsVB-178.htm)
- ArchiCAD 23 introduced a tool called «Отверстие» ("В ARCHICAD 23 появился новый Инструмент Отверстие"). It models openings, niches, chases and channels through individual elements at vertical, horizontal or sloped angles — [ArchiCAD 23 RU flyer, openbim.ru](https://openbim.ru/assets/files/ac23/AC23_Flyer_RUS_web.pdf)
- ArchiCAD RU uses «Проем в Стене» and «Пустой проем» (empty opening without a door/window) for wall openings placed via the door/window settings. It writes "проем" without «ё» — [ArchiCAD 25 RUS: Создание пустых проемов](https://help.graphisoft.com/AC/25/RUS/_AC25_Help/040_ElementsVB/040_ElementsVB-311.htm); [ArchiCAD 25 RUS: Проем в Стене](https://help.graphisoft.com/AC/25/RUS/_AC25_Help/150-1_DoorWindowCustomSettings/150-1_DoorWindowCustomSettings-17.htm)
- In ArchiCAD 29, imported IFC openings become «Отверстия» only if their hosts are walls, slabs, meshes, roofs or beams — [ArchiCAD 29 RUS: Импорт/Сохранение Отверстий в IFC](https://help.graphisoft.com/AC/29/RUS/_AC29_Help/040_ElementsVB/040_ElementsVB-267.htm)
- Renga has separate tools **«Отверстие»** and **«Проем»**. A release article captions a figure "Способы размещения отверстий в скатной крыше инструментами Отверстие (слева) и Проем (справа)" (◐: seen via a search snippet of the article) — [Renga release article: new Hole tool](https://rengabim.com/en/articles/new-hole-tool-improved-formulas-and-many-more-in-the-latest-renga-release/)
- In Renga you build walls, slabs (перекрытия) and stairs. The current help is tied to Renga 8.12 — [dtf.ru Renga overview](https://dtf.ru/software/5007385-renga-professional-bim-sistema-dlya-proektirovaniya)
- Revit 2027 Russian help names its modify tools: «Перенести» (Move), «Смещение» (Offset), «Выравнивание» (Align), «Повернуть» (Rotate), «Зеркало» (Mirror: "Инструмент «Зеркало» служит для симметричного отображения выбранного компонента модели относительно заданной линии"), «Вырезать», «Вставить», «Вставка с выравниванием», «Прикрепить»/«Отменить прикрепление» (Pin/Unpin) — [Revit 2027 RUS help: moving elements](https://help.autodesk.com/cloudhelp/2027/RUS/Revit-Model/files/GUID-B902060A-E2B1-4E20-A073-4A19BA3B3A14.htm)
- Revit's Mirror has two modes, "Mirror – Pick Axis" and "Mirror – Draw Axis", plus a "Copy" checkbox that decides whether the element is copied or moved. The Russian strings for these modes (commonly cited as «Отразить – выбрать ось» / «Отразить – нарисовать ось») were **not** confirmed — [Revit 2024 EN help: Mirror Elements](https://help.autodesk.com/cloudhelp/2024/ENU/Revit-Model/files/GUID-DF735D0A-A051-43C0-B21B-E8F6CBAFD5C3.htm)
- ArchiCAD RU titles the mirror topic «Зеркальное Отражение Элементов». Autodesk FormIt RU calls the tool «Зеркальное отражение» — [ArchiCAD 20 RUS](https://help.graphisoft.com/AC/20/RUS/AC20Help/02_Interaction/02_Interaction-75.htm); [FormIt RU: Зеркальное отражение](https://windows.help.formit.autodesk.com/ru/tool-library/mirror)
- An AutoCAD lecture names Move «Перенести» (plane-parallel translation) — [ZTU lecture](https://learn.ztu.edu.ua/mod/resource/view.php?id=239302)
- SketchUp RU: a Russian training document calls Push/Pull «Вдавить/Вытянуть» and Offset «Сдвиг». The official RU help calls Orbit «Орбита». The Russian name of the Tape Measure tool («Рулетка») was not confirmed — [RU SketchUp training doc](https://docs.google.com/document/d/1_6N_PtkNNdVbpMKhdpshApWwFtNho_nm/export?format=pdf); [SketchUp RU help: viewing model](https://help.sketchup.com/ru-ru/sketchup/viewing-model)
- Flutter Material RU (the built-in strings a Flutter app shows unless overridden) uses «Копировать», «Вырезать», «Вставить», «Выбрать все» (written without «ё»), «Удалить», «Сохранить», «Закрыть», «Поиск», «Ещё» (with «ё») — [flutter_localizations material_ru.arb](https://github.com/flutter/flutter/blob/master/packages/flutter_localizations/lib/src/l10n/material_ru.arb)
- The VAEX Смета mobile app (RuStore) calculates room **площади и периметр** from a drawn contour, accounting for windows and doors (◐ search summary) — [VAEX Смета, RuStore](https://www.rustore.ru/catalog/app/com.vaex.smeta.master)
- Microsoft's Russian style guide requires sentence-style capitalization for titles and headings (only the first word capitalized). That contradicts the Title Case seen in ArchiCAD RU — [Microsoft Russian Localization Style Guide §4.1.3](https://download.microsoft.com/download/c/8/a/c8ab1532-a15d-4f94-96c2-9740bc230757/rus-rus-StyleGuide.pdf)

### Inferences

**Editor and tool glossary (recommended UI labels)**

| Concept (EN) | Recommended RU UI label (len) | Official / CAD term (if different) | Common site / colloquial term | Notes: avoid / ambiguity | Uzbekistan usage |
|---|---|---|---|---|---|
| Wall | **Стена** (5) ✅ | Стена (ArchiCAD, Revit, Renga) ✅ | стенка ✱ | Separate type **Перегородка** (non-load-bearing partition) ✱ | Uzbek *devor*; Russian *stena*, *peregorodka* also heard ✱ |
| Opening in wall | **Проём** (5) ✅ | «Проем в стене», «Пустой проем» (ArchiCAD); «Проем» (Renga) ✅ | дырка, проём ✱ | Use **Проём** only for wall openings (door/window/empty). Use **Отверстие** for holes in slabs/roofs (ArchiCAD/Renga split) ✅. Decide on «ё» once (see §5) | *proyom* ✱ (no source found) |
| Hole (slab/roof/MEP) | **Отверстие** (9) ✅ | Отверстие (ArchiCAD 23+, Renga) ✅ | дырка, «под трубу» ✱ | Avoid «Отверстие» for windows/doors | — |
| Window | **Окно** (4) | Окно; оконный проём ✱ | окно | Window = object in a Проём | *deraza* (Uzbek) / *okno* ✱ |
| Door | **Дверь** (5) | Дверь; дверной проём ✱ | дверь | — | *eshik* (Uzbek) / *dver'* ✱ |
| Slab / floor structure | **Перекрытие** (10) ✅ | Перекрытие (Renga ✅; Revit/ArchiCAD ✱) | плита, плита перекрытия, «монолитка», «пустотка» (hollow-core) ✱ | «Плита» alone is ambiguous: slab foundation vs floor slab vs precast panel. Use **Перекрытие** for floors/ceilings and **Плита** only as a foundation type | *plita* ✅ (OLX: «плита ишлари»), *perekrytie* ✱ |
| Stair | **Лестница** (8) ✅ | Лестница; марш, площадка, проступь, подступенок ✅ | лестница, марш ✱ | — | *zina* (Uzbek) / *lestnitsa* ✱ |
| Column | **Колонна** (7) | Колонна ✱ | колонна, стойка ✱ | — | *kolonna* ✅ (OLX: «колонна») |
| Beam | **Балка** (5) | Балка; **ригель** (beam in a frame) ✱ | балка, ригель ✱ | Offer «Ригель» as a subtype or search alias | *rigel* ✅ (OLX: «ригель») |
| Roof | **Крыша** (5) | Крыша = structure; **кровля** = roofing layer ✱ | крыша, кровля ✱ | Drawing tool **Крыша**; estimate section **Кровля** (roofing work/materials) | *tom* (Uzbek) / *krovlya* ✱ |
| Dimension | **Размер** (6) | Размер, размерная линия ✱ | размер ✱ | — | *razmer* ✱ |
| Measure (tape) | **Измерить** (8) ✱ | Revit «Измерить» ✱; SketchUp «Рулетка» ✱ | рулетка, замер, «замерить» ✱ | Verb for the action; «Рулетка» is fine as an icon tooltip (subjective) | *ruletka*, *zamer* ✱ |
| Area | **Площадь** (7) | Площадь, м² | **квадраты**, квадратура («сколько квадратов?») ✱ | Show the unit **м²**. Builders price "за квадрат" (see §4) | *kvadrat* ◐ (OLX: priced per square meter) |
| Volume | **Объём** (5) | Объём, м³ | **кубы**, «куб бетона» ✱ | Show **м³** | *kub* ✱ |
| Text / note | **Текст** (5) | Текст ✱ | надпись ✱ | — | — |
| Select | **Выбрать** (7) | Flutter «Выбрать все» ✅ | выделить ✱ | Do not mix «Выбрать» and «Выделить» on one screen. «Выбрать» matches the Flutter default (subjective) | — |
| Copy | **Копировать** (11) ✅ | Копировать (Flutter) ✅ | — | Paired with «Вставить», «Вырезать» ✅ | Uzbek Flutter: «Nusxa olish» ✅ |
| Mirror | **Отразить** (8) ✱ | Revit help «Зеркало» ✅; ArchiCAD/FormIt «Зеркальное отражение» ✅ | «зеркально» ✱ | Avoid the adverb «Зеркально» and the long «Зеркальное отражение» (20). «Отразить» keeps the infinitive-verb pattern (subjective). Tooltip: «Отразить зеркально» | — |
| Array / repeat | **Массив** (6) ✱ | Массив (Revit/AutoCAD) ✱ | «размножить» ✱ | CAD jargon. Builders may not know it; consider **Размножить** (10) with tooltip «Копировать рядом» (subjective) | — |
| Move | **Переместить** (11) ✱ | Revit/AutoCAD «Перенести» ✅ | подвинуть, сдвинуть ✱ | Both work. Mobile/OS file UIs use «Переместить» ✱. Pick one; don't mix with «Перенести» | — |
| Rotate | **Повернуть** (9) ✅ | Повернуть (Revit) ✅ | — | — | — |
| Offset | **Смещение** (8) ✅ / «Сдвиг» | Revit «Смещение» ✅; SketchUp RU «Сдвиг» ◐ | — | Only if you implement offset | — |
| Split (cut wall in two) | **Разделить** (9) ✱ | Revit «Разделить элемент» ✱; AutoCAD «Разорвать» (break), «Расчленить» (explode) ✱ | разрезать ✱ | Avoid «Разбить» (means both break and explode). «Разделить» is clearest (subjective) | — |
| Delete | **Удалить** (7) ✅ | Удалить (Flutter) ✅; AutoCAD «Стереть» ✱ | — | — | Uzbek Flutter: «Olib tashlash» ✅ |
| Undo / Redo | **Отменить** / **Повторить** ✱ | standard RU OS/office strings ✱ | — | Flutter's Cancel is the noun **«Отмена»** ✅. Undo «Отменить» sits right next to it, so make Undo/Redo icon buttons with tooltips to avoid confusing «Отмена» with «Отменить» | — |
| Layers | **Слои** (4) ✱ | Слои (AutoCAD/nanoCAD/ArchiCAD) ✱; Revit uses visibility settings instead ✱ | — | For a simple show/hide panel, «Видимость» may be clearer to non-CAD builders (subjective) | *qatlam* (Uzbek) ✱ |
| Storey | **Этаж** (4) ✅ | ArchiCAD «Этаж» ✅; Revit/Renga «Уровень» ✱ | этаж, цоколь, подвал, мансарда, чердак ✱ | Avoid «Уровень» for storeys: it clashes with **уровень** = spirit level (tool) and **отметка** = elevation | *qavat* (Uzbek) / *etaj* ✱ |
| Elevation mark | **Отметка** (7) ✱ | отметка уровня (e.g., ±0,000) ✱ | «ноль», «нулевая отметка» ✱ | — | *otmetka* ✱ |
| Section | **Разрез** (6) ✱ | разрез ✱ | разрез ✱ | — | *razrez* ✱ |
| Plan | **План** (4) ✅ | «План Этажа» (ArchiCAD) ✅ | план ✱ | Write «План этажа» in sentence case, not ArchiCAD's Title Case ✅ (MS guide) | *plan* ✱ |
| Facade view | **Фасад** (5) ✱ | фасад ✱ | фасад ✱ | — | *fasad* ✱ |
| 3D view | **3D** (2) ✅ | «3D-окно» (ArchiCAD) ✅ | 3D, «объёмка» ✱ | Latin "3D" is universally understood | 3D ✱ |
| Grid axes | **Оси** (4) ✱ | координационные оси ✱ | оси, «ось А/1» ✱ | — | *o'q* / *os'* ✱ |
| Room | **Комната** (7) ✱ | Помещение (Revit/Renga) ✱; Зона (ArchiCAD) ✱ | комната ✱ | «Комната» suits homeowners and crews; «Помещение» is formal (subjective) | *xona* (Uzbek) ✱ |
| Foundation | **Фундамент** (9) ✱ | фундамент ✱ | фундамент, «лента» ✱ | — | *fundament* ✅ (OLX: «Фундамент қуйиш») |

Other inferences:
- **Labeling pattern (subjective):** use nouns for object-creation tools (Стена, Проём, Окно, Лестница) and infinitive verbs for edit actions (Копировать, Отразить, Повернуть, Удалить). This matches both CAD (nouns for elements) and Flutter/OS (infinitive verbs for actions).
- **Don't copy ArchiCAD's Title Case** («План Этажа», «Навесные Стены»). It conflicts with the Microsoft RU guide and normal Russian orthography.

### Gaps
- Could not retrieve Russian UI strings for **nanoCAD СПДС/BIM, КОМПАС-3D Строительная конфигурация, magicplan RU, Planner 5D RU, Remplanner, Дом-3D**. Searches returned no primary help pages. These need a manual check in the apps (install the RU UI and screenshot the toolbars).
- Exact Revit RU ribbon strings for Mirror modes («Отразить – выбрать ось»?), Array («Массив»?), Split («Разделить элемент»?), Level («Уровень»?), Measure («Измерить»?) were not confirmed. The Revit 2027 RU help confirmed «Зеркало» as the tool name.
- Renga's own help (help.rengabim.com/ru) returned only a search shell, and the Renga user-manual PDF download was blocked. Renga edit-command names are unverified.
- SketchUp RU «Рулетка» (Tape Measure) and «Перемещение» (Move) are unverified.

---

## 2. Construction materials, structural elements and masonry terms

### Takeaway
Official names come from GOST/SP (кирпич керамический 1НФ/1,4НФ, камень 2,1НФ; бетон класса B with legacy марка M; ЦЕМ I 42,5Н vs legacy М500; перемычки по ГОСТ 948-84; **антисейсмический пояс** and **сердечник** in СП 14.13330). Builders and sellers mostly use the short legacy or colloquial forms (одинарный/полуторный/двойной кирпич, М300, М500, газоблок, армопояс/сейсмопояс). The UI should show the **colloquial name first and the official grade in parentheses**, e.g. «Бетон М300 (B22,5)» and «Цемент М500 (ЦЕМ I 42,5Н)» (subjective).

### Cited Findings
- **Brick formats.** Single (одинарный, 1НФ) = 250×120×65 mm per ГОСТ 530-2012 (◐ product listing) — [bb.market listing](https://bb.market/us-en/stroitelstvo/kirpich-keramicheskij-ryadovoj-polnotelyj-odinarnyj-1nf-opt-d2211002597). The same listing's title shows the full commercial naming order: "Кирпич керамический рядовой полнотелый одинарный (1НФ)".
- One-and-a-half (полуторный) = 250×120×88 mm, marked **1,4НФ** in catalogs and sometimes called **«утолщённый»**. **2,1НФ** is typically 250×120×140 mm. **0,7НФ** («евро») = 250×85×65 mm — [KP.ru guide to facing-brick sizes](https://www.kp.ru/guide/razmery-oblitsovochnogo-kirpicha.html). The standard itself is «ГОСТ 530-2012 Кирпич и камень керамические. Общие технические условия» — [prg.kz copy of ГОСТ 530-2012](https://prg.kz/m/amp/document/31448969)
- **Wall thickness by brick count:** 250 mm = «в один кирпич», 380 mm = «в полтора кирпича» (250+120+10 mm joint), 510 mm = «в два кирпича». The values include 10 mm vertical joints, and the real load-bearing thickness should come from engineering calculation — [fb.ru: Кирпичная стена, толщина кладки](https://fb.ru/article/185710/kirpichnaya-stena-tolschina-kladki)
- **Concrete class vs mark.** ГОСТ 26633-91 Appendix 1 is titled "Соотношение между классами бетона по прочности на сжатие и растяжение и марками" — [zakonbase ГОСТ 26633](https://zakonbase.ru/content/part/464925?print=1). Sellers pair **B15 = М200** and **B22,5 = М300** (◐ commercial) — [productcenter.ru: Бетон М200 B15](https://productcenter.ru/products/107330/bieton-m200-v15). In Uzbekistan, ready-mix is advertised by mark, e.g. "бетон марки М300" — [prom.uz](https://www.prom.uz/ads/beton-marki-m-300/)
- **Cement marking.** Two systems coexist: modern ГОСТ 31108-2020 (e.g. **ЦЕМ I 42,5Н**, where ЦЕМ I = Portland clinker cement, 42,5 = compressive strength class, Н = normal hardening) and legacy ГОСТ 10178-85 (**ПЦ 500-Д0 / М500**). Bags often carry both. «Д0» means no additives — [Халва Медиа: как выбрать цемент](https://media.halvacard.ru/construction-and-repair/kak-vybrat-cement). The Uzbek producer AGMK markets "ЦЕМ II/А-И 32,5 Н" (modern GOST-style Russian marking) — [agmk.uz](https://agmk.uz/en/products/portlandtsement-obschestroitelnyj). A 25 kg bag is sold as "Портландцемент ЦЕМ I 42.5H Д0 (25кг)" — [21vek.by](https://www.21vek.by/tile_mixtures/i425h0_bck_6758525.html). OLX.uz cement listings include "Марка М450" and a per-kg price (◐) — [OLX.uz: sement](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/q-sement/)
- **Rebar.** Uzbek listings use class + diameter: "armatura 10 mm nlmk a500s" — [prom.uz](https://www.prom.uz/uz/ads/armatura-10-mm-nlmk-a500s/); "Armatura Eron 14 mm A400C — ulgurji narxda sotiladi!" (◐ snippet, Oct 2026) — [OLX.uz armatura category](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/metalloprokat-armatura/fittings/)
- **Lintels.** ГОСТ 948-84 «Перемычки железобетонные…» lists marks such as **2ПБ 10-1-п** with length/width/height and concrete/steel use — [meganorm ГОСТ 948-84](https://meganorm.ru/Data2/1/4294853/4294853404.htm). Series **1.038.1-1** includes plate lintels (плитные, issue 2) and bar and plate prestressed lintels (брусковые и плитные, issue 10) — [stroyinf catalog](https://files.stroyinf.ru/cat2/4294952817-0.htm). Kazakh marketplaces sell "перемычки 2ПБ 10-1п" by GOST mark — [satu.kz](https://satu.kz/Pb1.html)
- **Seismic belt and cores (official wording), СП 14.13330.2018 «Строительство в сейсмических районах»** — [SP 14.13330.2018 PDF](https://iisee.kenken.go.jp/worldlist/46_Russia/Russia_1.SP-14.13330.2018.pdf):
  - "устройством монолитных железобетонных обвязок (**антисейсмических поясов**)" (slab connections)
  - 6.14.11: "В уровне перекрытий и покрытий следует устраивать антисейсмические пояса по всем продольным и поперечным стенам…"
  - 6.14.12: belt height ≥ 150 mm, concrete class ≥ **В12,5**, longitudinal rebar ≥ **4d10** at 7–8 points and ≥ **4d12** at 9 points
  - 6.14.13: rebar mesh in masonry wall junctions (**арматурные сетки**). Walls must be topped with an "обвязочный железобетонный пояс, связанный с вертикальными железобетонными **сердечниками**". Brick piers only at 7 points with "**марка раствора** … не ниже М50"
  - "Вертикальные железобетонные элементы (**сердечники**) должны соединяться с антисейсмическими поясами"
  - 6.11.11: "**Перемычки** следует армировать пространственными каркасами…"
  - The SP was approved by Минстрой order No. 309/пр of 24 May 2018 and took effect 25 Nov 2018 (◐ from search summary of the same PDF).
- **Aerated / slag blocks in Uzbekistan.** OLX.uz has a category "kirpich-beton-penobloki / foam_and_aerated_blocks". Listings read "ГАЗОБЛОК сотилади!" and "Gazoblok / Газоблок" at ~8 700 сум per block (◐ snippets). A separate query "шлакоблок" exists in the Uzbek-Latin interface — [OLX.uz foam & aerated blocks](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/foam_and_aerated_blocks/); [OLX.uz шлакоблок (oz)](https://www.olx.uz/oz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/foam_and_aerated_blocks/q-шлакоблок/)
- **Formwork in Uzbek commerce.** Uzbek-language listings use "opalubka" (EMULSOL release oil "opalubka" uchun) and "armatura mexanik ulash muftasi" (rebar mechanical coupler) — [prom.uz EMULSOL](https://www.prom.uz/uz/ads/emulsol-opalubocnoe-maslo/); [prom.uz Promstroykontrakt](https://www.prom.uz/uz/company/promstroykontrakt/stroitelstvo/); [yellowpages.uz tag "Opalubka"](https://www.yellowpages.uz/uz/teg/opalubka)

### Inferences

**Materials and structure glossary**

| Concept (EN) | Recommended RU UI label (len) | Official term (if different) | Common site / slang | Notes: avoid / ambiguity | Uzbekistan usage |
|---|---|---|---|---|---|
| Brick (generic) | **Кирпич** (6) ✅ | кирпич керамический (ГОСТ 530-2012) ✅; кирпич силикатный ✱ | кирпич; «красный» (ceramic) / «белый» (silicate) ✱ | Material picker: type (керамический/силикатный) → format → kind (рядовой/лицевой) → полнотелый/пустотелый ✅ (order from listing) | Uzbek native **g'isht / ғишт** ✅ (OLX: «Ғишт териш»); *kirpich* also used ✱ |
| Single brick | **Одинарный (1НФ)** (15) ✅ | 1НФ, 250×120×65 ✅ | одинарный ✅ | Always show size in mm next to the format | *odinarniy* ✱ |
| 1.5 brick | **Полуторный (1,4НФ)** ✅ | 1,4НФ, 250×120×88 ✅ | полуторка ✱, утолщённый ✅ | — | *polutorniy* ✱ |
| Double brick | **Двойной (2,1НФ)** ✅ | камень керамический 2,1НФ, 250×120×140 ✅ | двойной ✅ | Officially a «камень», not a «кирпич». Show both | — |
| Euro brick | **Евро (0,7НФ)** ✅ | 0,7НФ, 250×85×65 ✅ | евро ✱ | — | — |
| Common vs facing brick | **Рядовой** / **Лицевой** ✱ | рядовой ✅ (listing), лицевой ✱ | **облицовочный** ✅ (KP.ru) | «Облицовочный» is the more common word in retail. Use it as a search alias | *oblitsovka* ✱ |
| Solid vs hollow | **Полнотелый** / **Пустотелый** ✅ (listing) / ✱ | — | «дырчатый» ✱ | — | — |
| Fire brick | **Шамотный** (9) ✱ | огнеупорный шамотный ✱ | шамотный, «печной» ✱ | Only for stoves/fireplaces | *shamot* ✱ |
| Wall thickness | **В 1 кирпич (250)** etc. ✅ | толщина стены 120/250/380/510 мм ✅ (120 = brick width, inferred) | **в полкирпича, в кирпич, в полтора кирпича, в два кирпича** ✅ | Show colloquial + mm: «в полтора кирпича · 380 мм» | Uzbek: *bir g'isht*, *bir yarim g'isht* ✱ |
| Aerated concrete block | **Газоблок** (8) ✅ | газобетонный блок; ячеистый бетон ✱ | газоблок, газобетон ✅ | Do **not** merge with **Пеноблок** (foam concrete; different density/price) ✱ | *gazoblok* ✅ |
| Foam concrete block | **Пеноблок** (8) ✅ | пенобетонный блок ✱ | пеноблок ✅ | — | *penoblok* ✅ (category name) |
| Expanded-clay block | **Керамзитоблок** (13) ✱ | керамзитобетонный блок ✱ | керамзитоблок ✱ | — | *keramzitoblok* ✱ |
| Slag/cinder block | **Шлакоблок** (9) ✅ | шлакобетонный блок ✱ | шлакоблок ✅ | — | *shlakoblok* ✅ |
| Masonry mortar | **Раствор** (7) ✱ | раствор строительный кладочный, цементно-песчаный; **марка раствора** М50…М200 ✅ (M50 in SP 14) | раствор ✱ | Show «Раствор М100» etc. | *rastvor* ✱ |
| Concrete | **Бетон** (5) | class **B** (B15, B22,5…) ✅; legacy **марка М** ✅ | «куб бетона», М300 ✅ | Show «М300 (B22,5)». Builders and Uzbek sellers quote марка ✅ | *beton M300* ✅ |
| Ready-mix truck | **Миксер** (6) ✱ | автобетоносмеситель ✱ | миксер ✱ | Delivery line item «Доставка миксером» ✱ | *mikser* ✱ |
| Rebar | **Арматура** (8) ✅ | class A500С/A400С ✅ (listings), A240 ✱; diameter in SP written **d12** ✅ | «арматура 12», «двенашка» ✱ (unverified) | Label diameters **Ø12 мм**. Never use slang numerals in UI | *armatura* ✅ |
| Tie wire | **Проволока** (9) ✱ | проволока вязальная ✱ | вязалка ✱ | Full name (19) too long; use subtitle | *sim* (Uzbek) / *provoloka* ✱ |
| Masonry mesh | **Сетка** (5) + subtitle «кладочная» | арматурные сетки (SP 14) ✅; сетка кладочная ✱ | сетка ✱ | «Кладочная сетка» = 15 chars; put the qualifier in the subtitle | *setka* ✱ |
| Lintel | **Перемычка** (9) ✅ | перемычки железобетонные (ГОСТ 948-84); брусковые (ПБ) / плитные (ПП) ✅ | перемычка ✅ | Picker by GOST mark (2ПБ 10-1-п…) plus «монолитная» option ✱ | *peremichka* ✱ (no listing found) |
| Formwork | **Опалубка** (8) ✅ | опалубка ✱ | опалубка, щиты ✱ | — | *opalubka* ✅ |
| Waterproofing | **Гидроизоляция** (13) ✱ | гидроизоляция ✱ | гидра ✱, рубероид ✱ | — | *gidroizolyatsiya* ✱ |
| Insulation | **Утеплитель** (10) ✱ | теплоизоляция ✱ | минвата, пенопласт, «пеноплекс» (brand) ✱ | Avoid brand names as category labels | *uteplitel* ✱ |
| Cement | **Цемент** (6) ✅ | ЦЕМ I 42,5Н / ЦЕМ II/А-И 32,5Н (ГОСТ 31108-2020) ✅ | **М500, М400, М450** ✅; «мешок» ✱ | Show «М500 (ЦЕМ I 42,5Н)». Unit = мешок with kg (25/50) ✅/✱ | *sement* ✅; per-kg pricing seen ✅ |
| Sand | **Песок** (5) ✱ | песок строительный ✱ | песок ✱ | — | Uzbek *qum* ✱ |
| Crushed stone | **Щебень** (6) ✱ | щебень (фракция 5–20…) ✱ | щебень ✱ | — | Uzbek *shag'al* ✱ |
| Sand-gravel mix | **ПГС** (3) ✱ | песчано-гравийная смесь ✱ | ПГС ✱ | Always expand on first use (tooltip) | ✱ |
| Bond beam / seismic belt | **Армопояс** (8) ✱ | **антисейсмический пояс**; **обвязочный железобетонный пояс** (SP 14) ✅ | армопояс ✱; **сейсмопояс** ✅ (UZ OLX) | Label «Армопояс». Add search aliases «сейсмопояс», «антисейсмический пояс». In 8–9-point zones, show SP 14 hints (h ≥ 150 мм, B12,5, 4Ø10/4Ø12) ✅ | *seysmopoyas* / сейсмопояс ✅ |
| RC core in masonry | **Сердечник** (9) ✅ | вертикальные железобетонные элементы (сердечники) ✅ | сердечник ✱ | — | *serdechnik* ✱ |
| Strip foundation | **Ленточный** (9) ✱ | ленточный фундамент ✱ | лента ✱ | Show as type under «Фундамент» | *lentochniy* ✱ |
| Slab foundation | **Плитный** (7) ✱ | плитный фундамент / фундаментная плита ✱ | плита, монолитная плита ✱ | Disambiguate from floor «Перекрытие» | *plita* ✅ |
| Sub-base cushion | **Подушка** (7) ✱ | песчаная (щебёночная) подготовка ✱ | подушка ✱ | — | *podushka* ✱ |
| Perimeter apron | **Отмостка** (8) ✱ | отмостка ✱ | отмостка ✱ | — | *otmostka* ✱ |
| Masonry | **Кладка** (6) ✅ | каменная кладка ✱ | кладка ✅ | — | Uzbek *g'isht terish* + «(Кладка)» ✅ |
| Joint pointing | **Расшивка** (8) ✱ | расшивка швов ✱ | расшивка (also the tool) ✱ | Ambiguous: process and tool | ✱ |
| Bond pattern | **Перевязка** (9) ✱ | перевязка швов; цепная/ложковая/тычковая ✱ | перевязка ✱ | — | ✱ |
| Screed | **Стяжка** (6) ✱ | стяжка ✱ | стяжка ✅ (OLX slug "styazhka") | — | *styajka* ✅ (slug) |
| Plaster | **Штукатурка** (10) ✱ | штукатурка ✱ | штукатурка ✅ (OLX slug "shtukaturka") | — | *shtukaturka* ✅ (slug) |

Other inferences:
- **Display rule (subjective):** colloquial grade first, official in parentheses: «Бетон М300 (B22,5)», «Цемент М500 (ЦЕМ I 42,5Н)», «Кирпич полуторный (1,4НФ) 250×120×88». Builders and Uzbek sellers quote марки (М300, М450, М500). The official class matters for engineers and sметчики.
- In Uzbekistan's seismic context (see §6), «сейсмопояс»/«армопояс» is everyday vocabulary. The app should treat it as a first-class element with its own estimate line (concrete m³ + rebar kg + formwork m²).

### Gaps
- No source confirmed that **«двенашка»** (Ø12 rebar) or **«десятка»** are used. Slang for rebar diameters is unverified; don't use it in the UI.
- Exact ГОСТ numbers and definitions for газобетон (ГОСТ 31360?), кладочный раствор (ГОСТ 28013?), арматура (ГОСТ 34028-2016?) and силикатный кирпич (ГОСТ 379?) were not fetched. Verify before quoting them in the app.
- No direct evidence of «peremichka» in Uzbek listings (searched; no hit). Uzbek Latin spellings marked ✱ are plausible transliterations, not observed usage.
- Bag weight norms for Uzbekistan cement (50 kg vs 25 kg) were not verified.

---

## 3. Builder tools and equipment: what builders actually call them

### Takeaway
Very little primary material was found online for tool slang. The only confirmed item is **«болгарка»** = colloquial for angle grinder (official УШМ). The rest of this table is ✱ domain knowledge to validate with real crews. Recommendation (subjective): use the colloquial name as the label and the official name as a subtitle/search alias, because estimating line items (rental, purchase) are read by crews, not engineers.

### Cited Findings
- **«Болгарка»** is the colloquial name for the angle grinder (угловая шлифовальная машина). The name comes from Bulgarian-made tools in the USSR in the 1970s — [Wiktionary: болгарка](https://en.wiktionary.org/wiki/%D0%B1%D0%BE%D0%BB%D0%B3%D0%B0%D1%80%D0%BA%D0%B0)
- A Russian/Uzbek research paper lists **"concrete mixer"** (бетономешалка) among equipment names that entered Uzbek through Russian — [Sharipova, TACE, Zenodo](https://zenodo.org/records/11170692/files/95-98%20Gulnoza%20Sharipova%20new%20MJST.pdf)
- Rebar-tying tools are sold as «вязальная проволока для арматуры» and «автоматический крюк для вязки арматуры» — [Yandex Market](https://market.yandex.ru/cc/9KfTim); [Yandex Market crochet hook](https://market.yandex.ru/offers/avtomaticheskii-kriuk-dlia-viazki-armatury-yato-yt-54233/887580625)

### Inferences

**Tools and equipment glossary (mostly ✱, to validate)**

| Concept (EN) | Recommended RU UI label | Official term | Site term | Notes | Uzbekistan usage |
|---|---|---|---|---|---|
| Laser distance meter | **Дальномер** (9) ✱ | лазерный дальномер ✱ | **лазерная рулетка** ✱, «лазерка» ✱ | For a "connect device" screen: «Лазерный дальномер» with subtitle «лазерная рулетка» | *dalnomer*, *lazernaya ruletka* ✱ |
| Tape measure | **Рулетка** (7) ✱ | рулетка измерительная ✱ | рулетка ✱ | — | *ruletka* ✱ |
| Spirit level | **Уровень** (7) ✱ | уровень строительный (пузырьковый) ✱ | уровень, ватерпас (old) ✱ | Conflicts with BIM «Уровень» (storey), so never use «Уровень» for storeys | *uroven'* ✱ |
| Laser level | **Лазерный уровень** (16) ✱ | построитель плоскостей ✱ | лазерник, «лазер» ✱ | Shorten to «Лазер. уровень» only if forced | ✱ |
| Optical level | **Нивелир** (7) ✱ | нивелир ✱ | нивелир ✱ | — | *nivelir* ✱ |
| Plumb line | **Отвес** (5) ✱ | отвес ✱ | отвес ✱ | — | *otves* ✱ |
| Straightedge rule | **Правило** (7) ✱ | правило ✱ | правило ✱ | — | *pravilo* ✱ |
| Trowel | **Кельма** (6) ✱ | кельма ✱ | мастерок ✱ | Both words are used; «мастерок» is more colloquial ✱ | *kelma*, *masterok* ✱ |
| Jointer (pointing tool) | **Расшивка** (8) ✱ | расшивка ✱ | расшивка ✱ | Same word as the process (§2) | ✱ |
| Concrete mixer (drum) | **Бетономешалка** (13) ✱ | бетоносмеситель ✱ | бетономешалка, «мешалка» ✱ | Not the same as «миксер» (truck) | *betonomeshalka* ✅ (via Russian, per Sharipova) |
| Poker vibrator | **Вибратор** (8) ✱ | глубинный вибратор ✱ | вибратор, «вибра» ✱ | — | *vibrator* ✱ |
| Angle grinder | **Болгарка** (8) ✅ | УШМ (угловая шлифовальная машина) ✅ | болгарка ✅ | Use «Болгарка» with «УШМ» as alias | *bolgarka* ✱ |
| Rotary hammer | **Перфоратор** (10) ✱ | перфоратор ✱ | перфоратор, «перф» ✱ | — | *perforator* ✱ |
| Scaffolding | **Леса** (4) ✱ | леса строительные ✱ | леса ✱ | «Подмости» = low trestles, a separate item | *lesa* ✱ |
| Low trestle platform | **Подмости** (8) ✱ | подмости ✱ | козлы ✱ | — | ✱ |

### Gaps
- No forum or corpus source was retrieved for actual crew slang frequency (forumhouse.ru searches returned nothing usable). Everything marked ✱ in this section needs validation, e.g. a 10-minute card-sort with 3–5 прорабы in Tashkent and 3–5 in Russia.
- Uzbek-language usage of tool names is unverified apart from «бетономешалка» (via Russian, per the Sharipova paper).

---

## 4. Estimating and finance UI terms, and how Russian estimating software names screens

### Takeaway
Official Russian estimating vocabulary centers on the **локальный сметный расчёт (смета)** (ЛСР, per Минстрой Методика 421/пр) and **ведомость объёмов работ**. Mobile estimating apps for private crews (RuStore/App Store) use plainer words: **смета, работы, материалы, расценки, надбавки, подытог, перерасход, отправить заказчику, экспорт в Excel**. In Uzbekistan, crews quote prices **per square meter** («за квадрат») and blocks/cement per piece/kg in **сум**. Recommendation: use the plain mobile-app vocabulary in the UI and keep official terms (ЛСР, ведомость) for PDF/export headers.

### Cited Findings
- Минстрой **Методика 421/пр** (order of 4 Aug 2020 No. 421/пр) defines the **ЛСР (локальный сметный расчёт)** forms. Form edits were made by order No. 55/пр of 30 Jan 2024 (base-index method form in Appendix 2), and a separate form exists for the resource method — [Consultant.ru: Методика 421/пр, ЛСР form](https://www.consultant.ru/document/cons_doc_LAW_362957/70334f8cf883e36a423056484c230adf433a840d/)
- Estimates are prepared from project documentation and **ведомости объёмов работ** listing work names, units and quantities (◐ Normacs expert answer citing п. 35) — [Normacs answer](https://www.normacs.info/answers/4484)
- In **Гранд-Смета**, a new estimate is created with the default name **«Локальная смета»** — [NNGASU methodological guide](https://bibl.nngasu.ru/electronicresources/uch-metod/construction_economics/877924.pdf)
- 1С construction accounting output forms include «Ведомость договорной цены», «Локальная смета (ресурсный)» and «Ресурсная ведомость (ресурсный)» — [pro1c.kz](https://pro1c.kz/articles/planirovanie-i-proizvodstvo-smr/opredelenie-obemov-vypolneniya-stroitelno-montazhnykh-rabot-v-1s-bukhgalteriya-stroitelnoy-organizats/)
- Smeta.RU's interface uses a «Содержание» tab and a «Проектная смета» mode (◐ search snippet) — [help.smeta.ru PDF](https://help.smeta.ru/5/5.pdf)
- Mobile estimating apps (◐ store-listing summaries, pages not opened):
  - **СметаБыстро**: volumes entered as formulas; **надбавки** as % of the **подытог по работам и материалам**; own **каталог расценок**; export to Excel — [RuStore](https://www.rustore.ru/catalog/app/ru.smetabystro.app)
  - **VAEX Смета**: aimed at **прорабы и бригады**; offline; **площади и периметр** from a drawn contour with windows/doors; automatic quantities of materials and works; **база расценок** — [RuStore](https://www.rustore.ru/catalog/app/com.vaex.smeta.master)
  - **Смета М2**: **расчёт материалов**, tracking **перерасход бюджета**, sending the estimate **заказчику** — [App Store RU](https://apps.apple.com/ru/app/%D1%81%D0%BC%D0%B5%D1%82%D0%B0-%D0%BC2-%D1%81%D1%82%D1%80%D0%BE%D0%B8%D1%82%D0%B5%D0%BB%D1%8C%D0%BD%D0%B0%D1%8F-%D1%81%D0%BC%D0%B5%D1%82%D0%B0/id1564662649)
  - **Калькулятор ремонта**: add **комнаты**, choose **виды работ** from a catalog, get a ready **смета** with the total — [RuStore](https://www.rustore.ru/catalog/app/com.repair.calculator)
  - **Прораб: Смета ремонта** exists, but no description was retrieved — [RuStore](https://www.rustore.ru/catalog/app/com.example.buildestimate)
- Uzbekistan pricing language: a contractor listing (Uzbek Cyrillic) offering «Ғишт териш (Кладка)… Фундамент қуйиш, сейсмопояс, колонна, ригель ва плита ишлари» says price is agreed **per square meter** by phone (◐ snippet; the listing now returns 404) — [OLX.uz listing](https://www.olx.uz/d/obyavlenie/vse-vidy-stroitelnyh-rabot-fundament-styazhka-shtukaturka-kladka-ID43HHr.html). Gas blocks are priced per piece in сум (~8 700) and cement per kg (◐) — [OLX.uz blocks](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/foam_and_aerated_blocks/); [OLX.uz sement](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/q-sement/)

### Inferences

**Estimating / finance glossary**

| Concept (EN) | Recommended RU UI label (len) | Official term | Site / colloquial | Notes | Uzbekistan usage |
|---|---|---|---|---|---|
| Estimate | **Смета** (5) ✅ | локальный сметный расчёт (смета), ЛСР ✅ | смета ✅ | Use «ЛСР» only in export headers | *smeta* ✱ |
| Bill of quantities | **Объёмы работ** (12) ✱ | ведомость объёмов работ ✅ | объёмы, «сколько кубов/квадратов» ✱ | Screen title; rows = работа + ед. изм. + кол-во ✅ | *hajm* (Uzbek) / *ob'yom* ✱ |
| Works / Materials (tabs) | **Работы** / **Материалы** ✅ | — | — | Matches mobile apps ✅ | *ishlar* / *materiallar* ✱ |
| Material consumption | **Расход** (6) ✱ | норма расхода ✱ | расход ✱ | e.g. «Расход: 400 шт./м³» ✱ | *rasxod* ✱ |
| Waste allowance | **Запас, %** (8) ✱ | потери и отходы (норм.) ✱ | «с запасом», бой (brick breakage) ✱ | Toggle «С запасом +10 %». Note the NBSP before % per MS docs ✅ (§5) | *zapas* ✱ |
| Unit rate | **Расценка** (8) ✅ | расценка, единичная расценка ✱ | цена за единицу, «за куб», «за квадрат» ✱/◐ | Show unit with price: «120 000 сум/м²» | «kvadrati …» ✱ |
| Markup | **Надбавка** (8) ✅ / **Наценка** ✱ | накладные расходы, сметная прибыль (official, ✱) | наценка ✱ | Mobile apps use «надбавки» ✅ | ✱ |
| Subtotal / Total | **Подытог** / **Итого** ✅/✱ | — | — | «Подытог по работам», «Подытог по материалам» ✅ | *jami* (Uzbek) ✱ |
| Purchase | **Закупка** (7) ✱ | — | закупка, «взять материал» ✱ | — | *xarid* ✱ |
| Delivery note / receipt | **Накладная** (9) / **Чек** (3) ✱ | товарная накладная ✱ | чек ✱ | «Фото чека» for receipt photos ✱ | *chek*, *nakladnoy* ✱ |
| Advance payment | **Аванс** (5) ✱ | аванс ✱ | аванс ✱ | — | *avans* ✱ |
| Paying the crew | **Оплата бригаде** (14) ✱ | расчёт с бригадой ✱ | «расчёт», «зарплата» ✱ | «Расчёт» is ambiguous (payment vs calculation). Avoid it as a lone button label | *usta haqi* ✱ |
| Foreman | **Прораб** (6) ✅ | производитель работ ✱ | прораб ✅ | — | *prorab* ✱ |
| Crew | **Бригада** (7) ✅ | — | бригада ✅ | — | *brigada*; master = *usta* ✱ |
| Contractor | **Подрядчик** (9) ✱ | подрядчик ✱ | — | — | *pudratchi* (Uzbek) ✱ |
| Client / owner | **Заказчик** (8) ✅ | заказчик ✅ | хозяин ✱ | «Отправить заказчику» ✅ | *buyurtmachi*, *xo'jayin* ✱ |
| Remaining | **Остаток** (7) ✱ | — | остаток ✱ | — | *qoldiq* ✱ |
| Overrun | **Перерасход** (10) ✅ | — | перерасход ✅ | — | ✱ |
| Plan vs actual | **План** / **Факт** ✱ | — | план/факт ✱ | Column headers | ✱ |
| Export | **Экспорт в Excel** ✅ / «PDF» | — | — | Apps advertise Excel export ✅ | ✱ |

### Gaps
- Could not open the Гранд-Смета or Smeta.RU UIs to list real button/screen names beyond «Локальная смета», «Содержание», «Проектная смета». The «Строй-Смета» and «ПроСмету» names from the brief were not found.
- No Uzbek-specific estimating norms (e.g., ШНК/КМК estimate documents) were researched.
- Labor pricing units used in Russia (за м², за м³, за шт.) are assumed by analogy, not sourced.

---

## 5. Russian mobile UI microcopy conventions (verbs, casing, units, numbers, plurals, length)

### Takeaway
Hard facts: Russian uses **decimal comma** and **no-break-space thousands grouping** («6 400», «1,39»; CLDR group separator is U+00A0). Units go after the number with a no-break space («5 кг»). Use **sentence case**. Plurals need **four CLDR categories** (one/few/many/other). Mobile platform defaults (Flutter Material RU) use **infinitive verbs** for actions (Сохранить, Удалить, Копировать, Закрыть, Продолжить), but **nouns for Cancel/Search** («Отмена», «Поиск») and Cyrillic «ОК». CLDR gives Russian names for UZS but **no Russian symbol**, so «сум» must be supplied by the app.

### Cited Findings
- **CLDR `ru` number symbols:** decimal = «,». Group = U+00A0 NO-BREAK SPACE (byte check: `c2 a0`). Decimal pattern `#,##0.###`. Currency pattern `#,##0.00 ¤`. Short forms `0 тыс.` and `0 млн` — [CLDR ru numbers.json](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/ru/numbers.json)
- **CLDR `ru` plural rules:** *one* = i%10=1 and i%100≠11 (1, 21, 31…). *few* = i%10=2..4 and i%100≠12..14 (2–4, 22–24…). *many* = 0, 5–20, 25–30… *other* = decimals (0,5; 1,5…) — [CLDR plurals.json](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-core/supplemental/plurals.json)
- Flutter Material RU shows the pattern in practice: "Выбран 1 объект" / "Выбрано $n объекта" (few) / "Выбрано $n объектов" (many) / "Выбрано $n объекта" (other), and "Остался 1 символ" / "Осталось $n символа/символов" — [material_ru.arb](https://github.com/flutter/flutter/blob/master/packages/flutter_localizations/lib/src/l10n/material_ru.arb)
- **Flutter Material RU button strings:** «Отмена» (Cancel), «ОК» (Cyrillic О+К; bytes `d0 9e d0 9a`), «Сохранить», «Удалить», «Закрыть», «Продолжить», «Копировать», «Вырезать», «Вставить», «Выбрать все», «Поиск», «Ещё», «Назад», «Поделиться», «Найти», «Удалить текст» — [material_ru.arb](https://github.com/flutter/flutter/blob/master/packages/flutter_localizations/lib/src/l10n/material_ru.arb). **Cupertino RU** also uses «Отмена», «Назад», «Копировать», «Вставить», «Выбрать все», «Закрыть меню», and abbreviates time units as «мин.» and «сек.» (with period) — [cupertino_ru.arb](https://github.com/flutter/flutter/blob/master/packages/flutter_localizations/lib/src/l10n/cupertino_ru.arb)
- **Microsoft Russian Localization Style Guide** — [PDF](https://download.microsoft.com/download/c/8/a/c8ab1532-a15d-4f94-96c2-9740bc230757/rus-rus-StyleGuide.pdf):
  - §4.1.3: sentence-style capitalization for titles and headings (only the first word capitalized). UI element names are quoted in «ёлочки», e.g. "Нажмите кнопку «Изменить»", «Далее», «Готово».
  - §4.1.13: use **no-break spaces** as the thousands separator ("10 000"), between a number and its unit ("5 кг", "5 °C"), before % in documentation ("10 %"), and in "и т. д.".
  - §4.1.1: abbreviate in UI (mainly buttons/options) only with accepted abbreviations: стр., рис., см., и т. д., и т. п., **ч, мин, с** (no period), г., мес., нед., дн., КБ, МБ. "Don't abbreviate other words arbitrarily."
  - Numerals are preferred with currencies and units of measurement.
  - For AI/Copilot prompts: avoid the formal «вы», avoid bureaucratic style (канцеляризмы), be concise. Start with an infinitive, noun or imperative depending on fluency.
  - The guide was rewritten from a formal, impersonal style to a livelier, personal tone — [Microsoft Terminology blog](https://learn.microsoft.com/en-us/archive/blogs/terminology/new-writing-style-in-russian-microsoft-products)
- A Habr UX article (2000s-era) argues a search button should say what the user wants done ("Искать"/"Найти!") — [Habr 27165](https://habr.com/en/articles/27165)
- **CLDR currencies, `ru`:** UZS displayName «узбекский сум» (one), «узбекских сума» (few), «узбекских сумов» (many), «узбекского сума» (other). **No `symbol` entry** for UZS. RUB symbol «₽» — [CLDR ru currencies.json](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/ru/currencies.json). In `uz` the UZS symbol is «soʻm»; in `uz-Cyrl` it is «сўм» — [CLDR uz currencies](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/uz/currencies.json); [CLDR uz-Cyrl currencies](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/uz-Cyrl/currencies.json)

### Inferences

**Microcopy rules for the app (subjective unless marked ✅)**

| Topic | Rule | Example | Basis |
|---|---|---|---|
| Action buttons | Infinitive verb, 1–2 words | Сохранить, Добавить, Удалить, Отправить, Рассчитать | ✅ Flutter RU pattern; ✱ convention |
| Cancel / dismiss | Use **«Отмена»** (noun) to match the Flutter system dialogs | Отмена | ✅ Flutter RU |
| Done (finish editing) | **«Готово»** | Готово | ✅ appears in MS guide examples; ✱ iOS usage |
| Apply filter/settings | **«Применить»** | Применить | ✱ |
| OK | Cyrillic **«ОК»**, not Latin "OK" | ОК | ✅ Flutter RU bytes |
| Undo vs Cancel | Undo = icon + tooltip «Отменить действие»; Cancel = «Отмена». Never show "Отменить" and "Отмена" side by side | — | ✱ (derived from Flutter's «Отмена») |
| Imperative («Сохраните») | Only in instructions/hints, not on buttons | «Нарисуйте контур комнаты» | ✱; MS guide uses imperative in procedures ✅ |
| Capitalization | Sentence case everywhere | «План этажа», «Объёмы работ» | ✅ MS §4.1.3 |
| Thousands | NBSP U+00A0 (CLDR `ru`); never a comma or period | 6 400; 1 250 000 | ✅ CLDR / MS |
| Decimals | Comma | 1,39; 0,25 м³ | ✅ CLDR |
| Number + unit | NBSP between them | 380 мм; 12,5 м²; 3 т | ✅ MS §4.1.13 |
| Units | мм, см, м, м², м³, кг, т, шт., л; time «ч», «мин», «с» | 2,4 м³ бетона | ✅ MS (ч/мин/с, кг); ✱ (шт., м², м³, т). Note Cupertino RU writes «мин.», «сек.», a conflict with MS |
| Large money | «тыс.» / «млн» (CLDR short forms); «млрд» ✱ | 6,4 млн сум | ✅ CLDR |
| Percent | NBSP before % in documents. In tight UI chips, «10%» without a space is common but non-standard ✱ | Запас 10 % | ✅ MS |
| Currency (UZ) | Supply your own symbol: «сум» after the number; no decimals (tiyin unused ✱) | 8 700 сум; 1,2 млн сум | ✅ CLDR has no RU symbol for UZS; ✱ formatting choice |
| Currency (RU) | «₽» or «руб.» after the number | 6 400 ₽ | ✅ CLDR symbol |
| Plurals | Implement one/few/many/other for every countable noun | 1 стена · 2 стены · 5 стен · 1,5 стены; 1 этаж · 3 этажа · 5 этажей; 1 мешок · 2 мешка · 5 мешков; 1 кирпич · 3 кирпича · 10 кирпичей | ✅ CLDR rules + Flutter examples |
| Avoid plurals in labels | Prefer «Стен: 5», «Кол-во: 5 шт.» in tight spaces | Стен: 5 | ✱ common workaround |
| Abbreviations | Only standard ones (кол-во, ед. изм., шт., тыс., млн). Don't invent («Перекр.», «Гидроиз.») | — | ✅ MS "don't abbreviate arbitrarily" |
| «ё» | Pick a rule and apply it consistently. Recommend writing «ё» in labels where it disambiguates (всё/все, объём, проём, расчёт, ещё). Flutter itself is inconsistent («Ещё» but «Выбрать все») | Объём, Проём | ✱ subjective |
| Address form | Prefer impersonal/infinitive labels. In messages use «вы» (lowercase), friendly and without канцелярит | «Смета сохранена», «Добавьте первую стену» | ✅ MS tone guidance; ✱ lowercase «вы» |
| Length budget | Target ≤ 14 characters per button. Use icon + 1 word on the toolbar, full phrase in the tooltip | «Армопояс» (8) vs «Антисейсмический пояс» (21) | ✱ |

Dart/Flutter implementation notes (✱ to verify in tests): `NumberFormat.decimalPattern('ru')` should emit U+00A0 grouping per CLDR. Since CLDR `ru` has no UZS symbol, `NumberFormat.currency(locale: 'ru', name: 'UZS')` will likely print "UZS". Use `NumberFormat.currency(locale: 'ru', symbol: 'сум', decimalDigits: 0)` or a custom pattern. Some fonts render U+00A0 poorly. Test on real devices.

### Gaps
- **Gosuslugi design system / editorial policy** (guides.gosuslugi.ru) could not be fetched (the page returned only the portal title). **Yandex, Sber and T-Bank (Tinkoff)** UI-text guides were not retrieved. The "infinitive on buttons" rule is therefore supported by observed platform strings (Flutter RU) and convention, not a quoted Russian design-system rule.
- The Apple iOS Russian system strings (e.g., whether Cancel is «Отменить» or «Отмена», Edit «Изменить» or «Править») were not verified.
- The "Russian text is ~20–30 % longer than English" claim was not sourced in this research.
- ГОСТ 8.417 (units) and the ГОСТ 21.xxx СПДС drawing conventions (mm on plans, elevations in m with 3 decimals «+3,000») were not fetched.

---

## 6. Uzbekistan specifics: Russian in Uzbek construction trades, mixed terms, currency

### Takeaway
Russian technical vocabulary is entrenched in Uzbek construction. A Tashkent architecture/civil-engineering researcher calls Russian construction terms "relics" in Uzbek and says Uzbek replacements are adopted "very slowly". Real listings mix scripts and languages in one sentence: an Uzbek verb or frame with Russian nouns («Фундамент қуйиш, сейсмопояс, колонна, ригель ва плита ишлари», «ГАЗОБЛОК сотилади!», «Armatura … A400C — ulgurji narxda sotiladi!»). Some core words stay Uzbek («ғишт/g'isht» = brick, «ғишт териш» = bricklaying, glossed «(Кладка)»). A Russian-first UI is appropriate. It should add **Uzbek-Latin/Cyrillic search aliases**, use **«сум»** as an app-supplied currency label, and treat **seismic elements** as standard.

### Cited Findings
- Gulnoza Sharipova (Tashkent University of Architecture and Civil Engineering) — [Zenodo PDF](https://zenodo.org/records/11170692/files/95-98%20Gulnoza%20Sharipova%20new%20MJST.pdf):
  - "the Uzbek architecture-construction field terminology has been enriched through the acquisition of Russian-international terms"
  - Builders and employers "for more than half a century were mainly representatives of the Russian language, many Russian construction and architectural terms have become 'relics' for the Uzbek language"
  - Causes: imported equipment and materials, and that "the master builders spoke Russian and the language of construction and architecture was also Russian"
  - Uzbek equivalents are offered, "but their adoption by the living language is very slow"
  - Examples of terms entering via Russian: building parts (roof, basement, attic, floor…), equipment (concrete mixer), products (brick, stone)
- **Mixed-language listing** (Uzbek Cyrillic + Russian nouns): «Bajaradigan xizmatlarimiz: Ғишт териш (Кладка)… Бетон ва монолит ишлари: Фундамент қуйиш, сейсмопояс, колонна, ригель ва плита ишлари». The URL slug is Russian transliteration: "vse-vidy-stroitelnyh-rabot-fundament-styazhka-shtukaturka-kladka". Pricing is negotiated per m² (◐ snippet; listing now 404) — [OLX.uz](https://www.olx.uz/d/obyavlenie/vse-vidy-stroitelnyh-rabot-fundament-styazhka-shtukaturka-kladka-ID43HHr.html)
- **Product listings:** «ГАЗОБЛОК сотилади!» (Russian noun + Uzbek verb, Cyrillic) and «Gazoblok / Газоблок» (dual script), ~8 700 сум/шт. — [OLX.uz aerated blocks](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/foam_and_aerated_blocks/). «Armatura Eron 14 mm A400C — ulgurji narxda sotiladi!» (Uzbek Latin + Russian class mark) — [OLX.uz armatura](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/metalloprokat-armatura/fittings/). "armatura 10 mm nlmk a500s" — [prom.uz](https://www.prom.uz/uz/ads/armatura-10-mm-nlmk-a500s/). "opalubka" in Uzbek product text — [prom.uz](https://www.prom.uz/uz/ads/emulsol-opalubocnoe-maslo/)
- The OLX.uz category paths themselves are Russian transliterations (kirpich-beton-penobloki, metalloprokat-armatura, tovari-dlya-stroitelstva-remonta), even in the Uzbek-Latin interface (/oz/) — [OLX.uz oz interface](https://www.olx.uz/oz/dom-i-sad/tovari-dlya-stroitelstva-remonta/metalloprokat-armatura/fittings/tashmore/)
- Uzbek cement is sold with Russian-GOST-style marking «ЦЕМ II/А-И 32,5 Н» (AGMK) — [agmk.uz](https://agmk.uz/en/products/portlandtsement-obschestroitelnyj). OLX cement ads use «Марка М450» — [OLX.uz](https://www.olx.uz/dom-i-sad/tovari-dlya-stroitelstva-remonta/kirpich-beton-penobloki/q-sement/)
- **Seismicity:** 2025 government measures list Andijan at 9 points and Tashkent, Bukhara, Namangan, Nurafshon, Fergana at 8–9 points (MSK-64) — [Kun.uz, 18 Nov 2025](https://kun.uz/en/news/2025/11/18/uzbekistan-approves-new-measures-to-improve-seismic-resistance-of-buildings-and-structures). Uzbekistan's seismic code is cited as КМК/ШНК 2.01.03-19 "Construction in seismic areas" (◐ designation inconsistent across sources) — [E3S Conferences 2024](https://www.e3s-conferences.org/10.1051/e3sconf/202447401020)
- **Flutter Uzbek (Latin) defaults** for reference if an Uzbek UI is added later: «Bekor qilish» (Cancel), «Saqlash» (Save), «Olib tashlash» (Delete), «Nusxa olish» (Copy), «Hammasi» (Select all), «OK» — [material_uz.arb](https://github.com/flutter/flutter/blob/master/packages/flutter_localizations/lib/src/l10n/material_uz.arb)
- **Currency:** CLDR `ru` names UZS «узбекский сум» (plural «сума»/«сумов») with no symbol. CLDR `uz` symbol is «soʻm», and `uz-Cyrl` is «сўм» — [CLDR ru](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/ru/currencies.json); [CLDR uz](https://github.com/unicode-org/cldr-json/blob/main/cldr-json/cldr-numbers-full/main/uz/currencies.json)

### Inferences
- **Russian-first UI is justified** for technical labels in Uzbekistan. Uzbek speakers already use the Russian nouns (фундамент, сейсмопояс, колонна, ригель, плита, кладка, газоблок, арматура, опалубка), so a Russian label is recognisable even to Uzbek-dominant crews (inference from the Sharipova paper and the listings).
- **Search must be script-agnostic (strongly recommended):** index every material/element under Russian Cyrillic, Uzbek Latin transliteration and Uzbek Cyrillic, e.g. «Газоблок» ↔ gazoblok ↔ газоблок; «Опалубка» ↔ opalubka; «Арматура» ↔ armatura; «Перемычка» ↔ peremichka/peremychka ✱; «Армопояс» ↔ armopoyas ✱ ↔ сейсмопояс ↔ seysmopoyas ✱; «Кирпич» ↔ g'isht/ғишт ✅; «Цемент» ↔ sement ✅; «Кладка» ↔ g'isht terish ✅; «Заливка фундамента» ↔ fundament quyish ✅.
- **Uzbek words to keep in mind** even in a Russian UI (subjective): *usta* (master builder), *g'isht* (brick), *qum* (sand ✱), *quyish* (pouring), *terish* (laying), *kvadrat/kub* for pricing units ✱.
- **Currency display:** «8 700 сум», «1,2 млн сум», «450 тыс. сум» (✱ convention; CLDR supplies «тыс.»/«млн» ✅). Settings: currency picker «Сум (UZS)» / «Рубль (RUB)»; per-project currency.
- **Seismic defaults:** for UZ projects, default the masonry wall template to include «Армопояс» at each floor level and «Сердечники» at corners. Show the СП 14.13330 hints (belt h ≥ 150 мм, B12,5, 4Ø12 at 9 points) as *reference*, not as a design calculation. Uzbek КМК values may differ (not verified).
- **Pricing units in UZ:** crews negotiate labor «за квадрат» (per m²) ◐. Materials are priced per piece (blocks), per kg (cement) or per tonne/piece (rebar ✱), so the estimate engine needs per-unit price fields that match these units.

### Gaps
- No quantitative survey was found on whether Uzbek builders **prefer Russian over Uzbek** for technical terms. The evidence is qualitative (one academic paper plus listing language).
- «Peremichka» was not observed in any Uzbek listing retrieved (search found none). «Armopoyas»/«seysmopoyas» in Uzbek Latin spelling was not observed; only Cyrillic «сейсмопояс» was seen.
- The text of Uzbekistan's seismic norm (КМК/ШНК 2.01.03-19) and its exact term for the seismic belt were not retrieved (lex.uz / Minstroy UZ not fetched).
- No source was found for Russian-language Uzbek media money formatting («млн сумов» vs «млн сум»). The choice is a style decision.
- stroyka.uz and the uz.olx Uzbek-Latin pages were not opened in full. Most UZ evidence comes from search snippets of listings (◐).
