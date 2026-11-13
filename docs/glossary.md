# Glossary

Terms used in code (English), in the UI (Russian first) and by builders in Uzbekistan.

| Code | Russian (UI) | Uzbek | Meaning |
|---|---|---|---|
| job | объект | obyekt | One construction site; one `.job` file |
| op log | журнал изменений | o‘zgarishlar jurnali | Property-level changes of a job in order; undo, history and sync read it (ADR 0003, 0007) |
| part | часть | qism | House, summer kitchen, fence, yard inside a job |
| level | этаж | qavat | Storey, including basement (подвал) and foundation; walls stand on one, heights measured from its floor |
| wall | стена | devor | Masonry or block wall with thickness in bricks; its height is the masonry, below the ring beam |
| location line | линия привязки | bog‘lash chizig‘i | The wall line whose points are stored: centre, left or right face |
| sketch | набросок | qoralama | A stroke drawn with a finger and read as walls (ADR 0012) |
| exact length | точный размер | aniq o‘lcham | A wall length that was typed or measured; a sketched wall's is written with «≈» until then |
| wall join | сопряжение стен | devorlar tutashuvi | How wall ends meet: a mitre at a corner, slices around a shared point |
| butt joint | примыкание | tirab ulash | A wall ending against the face of another wall |
| masonry unit | кирпич, блок | g‘isht, blok | A brick or block as laid: length along the wall, width across, course height |
| wythe | ряд в полкирпича | yarim g‘isht qatlami | One unit width across the wall; a 380 mm brick wall has three |
| bed joint | горизонтальный шов | gorizontal chok | Mortar between courses, 12 mm by СП 70 |
| head joint | вертикальный шов | vertikal chok | Mortar between units in a course, 10 mm by СП 70 |
| mortar | раствор | qorishma | Cement-sand mix that fills the joints |
| opening | проём | teshik | Window, door or empty opening in a wall |
| hole | отверстие | teshik (plitada) | Opening through a slab or roof, not a wall |
| lintel | перемычка | peremichka | Beam over an opening (ГОСТ 948) |
| bearing | опирание | tayanish | Length of a lintel or slab resting on the masonry on each side |
| ring beam | армопояс | armopoyas, seysmopoyas | Seismic reinforced-concrete belt (КМК 2.01.03-19) |
| junction mesh | сетка в узле сопряжения | tutashma to‘ri | Bed-joint mesh where walls meet, every 700 or 500 mm of height (КМК 2.01.03-19, 3.5.12) |
| core column | сердечник | o‘zak | Reinforced-concrete core in masonry at corners and piers |
| strip footing | ленточный фундамент | lentali poydevor | Continuous foundation under walls |
| slab | плита | plita | Floor or roof slab |
| take-off | расход материалов | material sarfi | Quantities derived from the model |
| estimate | смета | smeta | Priced snapshot of quantities, labour and markup |
| ledger | учёт расходов | xarajatlar | Purchases, labour and payments on site |
| rule | правило расчёта | hisob qoidasi | Luau script in `content/rules` that turns quantities into material lines |
| norm profile | нормативная база | me’yorlar bazasi | Set of rules: UZ (КМК/ШНК), RU (СП), EU (EN) |
| plumb bob | отвес | otves | The brand: a tool that checks the true vertical |
