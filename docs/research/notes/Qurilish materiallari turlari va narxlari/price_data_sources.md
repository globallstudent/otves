# Construction material catalogs & price data sources — Uzbekistan and Russia (for a mobile estimating app)

_Research date: 2026-11-06. All "App use:" notes are my inferences, not facts from sources. "(search summary)" means the fact came from a search-result digest of that page rather than a full page fetch, so it is somewhat less reliable._

---

## Q1. Uzbekistan — official construction price bodies, resource classification and price lists

### Takeaway
Uzbekistan has largely replaced paper "current price" bulletins with one state digital platform: **«Qurilish resurslari milliy klassifikatori»** (National Construction Resources Classifier), now at **mkinfo.uz**. It combines ~180k+ coded materials with prices from manufacturers, the commodity exchange (UZEX), tax e-invoices (MXIK/ИКПУ), customs, procurement and statistics, and it is the legal basis for estimate starting prices on state-funded projects. A 2026 decree moves the platform to a new **"Shaharsozlik sohasida iqtisodiy islohotlar va narxlarni shakllantirish respublika markazi"** under the Ministry of Construction and Housing and Communal Services. I found no documented public API, no open-data licence and no downloadable periodic price bulletin for 2025–2026.

### Cited Findings
**Legal basis and who runs it**
- Cabinet of Ministers decree **No. 231 of 23.04.2024**, «Qurilish sohasidagi normativ hujjatlarni xalqaro standartlar bilan uygʻunlashtirish chora-tadbirlari toʻgʻrisida», in force from 24.04.2024. Annex 4 was amended by decree No. 627 of 06.10.2025. It tasks the Technical Regulation and Standardization Research Institute (TMSITI) with introducing the classifier and running the "narxlar elektron platformasi" (electronic price platform). Resources are grouped into groups and sections, and each item gets its own identification number — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291)
- Decree 231 says the platform is funded by "foydalanuvchilarning toʻlovlari" (user fees), Institute funds and Fund money. Fees from manufacturers and suppliers for goods listed on the classifier or platform are a funding source. So the platform is a paid service for suppliers, not open data — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291)
- Decree 231, Annex 4 implementation scheme (data feeds into the platform). This is the most useful item for the app, because it lists where the prices come from:
  1. The Ministry of Construction adds new resources to the classifier. Ongoing.
  2. The platform is built inside the **"Shaffof qurilish" national information system**.
  3. The **Commodity-Raw Materials Exchange (UZEX)** posts weekly price quotations for construction resources automatically ("haftalik kotirovkalarni avtomatik tarzda"). Ongoing.
  4. The Customs Committee sends data on construction materials cleared for free circulation.
  5. Businesses register with ERI (e-signature) and post technical and price information.
  6. The Ministry of Finance feeds construction resource data from the public procurement e-system.
  7. The State Tax Committee feeds data from the unified electronic product catalog (MXIK).
  8. The National Statistics Committee supplies names, units and prices from the producer price report (form 12).
  9. The technical regulation agency posts technical parameters from conformity certificates.
  — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291)
- President's videoconference on 06.07.2022: the "Qurilish resurslari klassifikatori" platform was launched with the task of forming "100 mingdan ziyod qurilish materiallari va xizmatlarining bozor narxini" (market prices for more than 100k construction materials and services). The same meeting ordered an "elektron katalog" of standard designs in "Shaffof qurilish" — [president.uz/oz/lists/view/5319](https://president.uz/oz/lists/view/5319)
- Aniq.uz, 7 May 2024: the platform (then at **catalog.tmsiti.uz**) held **181,740 materials and products, 1,029 machines and mechanisms, 352 small-mechanization items, 3,685 equipment items and 389 construction works**. Manufacturers log in with ERI and enter prices for their own products. It is used to form "dastlabki boshlang'ich narxlari" (initial starting prices) of social-infrastructure objects under state programs. It was created under Presidential decree PF-6119 of 27.11.2020. About 100 enterprises had posted products by that date — [aniq.uz](https://aniq.uz/uz/yangiliklar/qurilish-materiallari-milliy-klassifikatori-elektron-platformasi-tadbirkorlar-uchun-yana-bir-qulaylik)
- From 1 July 2024, the estimated cost of objects is formed on the basis of the "Qurilish resurslari milliy klassifikatori" platform data. For road, drinking-water and bridge projects, the "resource method" is replaced by a "volume method" (search summary) — [spot.uz 2024-06-27](https://www.spot.uz/oz/2024/06/27/1st-july-changes/); [president.uz/oz/lists/view/7002](https://president.uz/oz/lists/view/7002)
- Spot.uz, 9 June 2026: a presidential decree creates the **"Shaharsozlik sohasida iqtisodiy islohotlar va narxlarni shakllantirish respublika markazi"** under the Ministry of Construction and Housing and Communal Services. Its tasks: "“Qurilish resurslari milliy klassifikatori” platformasini yuritadi, narx belgilash tizimini raqamlashtiradi hamda qurilish materiallari va resurslari bozoridagi vaziyatni kuzatib boradi". The article gives no decree number or start date — [spot.uz/oz/2026/06/09/construction-eco](https://www.spot.uz/oz/2026/06/09/construction-eco)
- Spot.uz, 23 Oct 2026: a government decree approved rules for contractors' "other costs" when calculating contractual current prices:
  - Other costs are 20% of direct costs, plus 7% necessary profit. Production-related other costs are 2.5% (5% in mountainous, desert and hard-to-reach areas).
  - These are calculated, checked and approved electronically through the "Shaffof qurilish" national information system, and published there by organization.
  - From 2027, a new procedure for initial value applies to projects funded by the budget or by entities with ≥50% state share.
  — [spot.uz/oz/2026/10/23/contract-organizations](https://www.spot.uz/oz/2026/10/23/contract-organizations)

**mkinfo.uz (current home of the classifier)**
- https://mkinfo.uz/ serves a React single-page app titled «Qurilish resurslari milliy klassifikatori». Search snippets describe the /classifier page as a searchable product table with codes, names, GOST standards and units, with sections for materials and equipment, construction works and machinery — [mkinfo.uz/classifier](https://www.mkinfo.uz/classifier). Note: on 2026-11-06 the www. host returned an expired TLS certificate to WebFetch, while https://mkinfo.uz/ loaded.
- I inspected the site's public JS bundle (`https://mkinfo.uz/assets/index-BRGLl3ss.js`, fetched 2026-11-06).
  - Backend base URL: `https://mk.shaffofqurilish.uz/api`.
  - Route names include `/classifiers`, `/catalog`, `/get-resources`, `/get-catalog-resources`, `/get-birja-materials` (exchange materials), `/get-soliq-average` and `/get-soliq-average-history` (tax/e-invoice average prices), `/estat-sdmx-data` (statistics SDMX), `/wages`, `/regions`, `/districts`, `/manufacturers`, `/certificates`, `/create-material-ads`, `/get-material-ads`, `/create-material-request`, `/get-request-excel` and `/last-available-period`.
  - The bundle also links to `https://tmsiti.uz/standards` and `https://t.me/mkinfo_support`.
  - This API is **undocumented and I did not call it**; it is an internal frontend API, not a published developer API. — own inspection of [mkinfo.uz](https://mkinfo.uz/)
- Buxgalter.uz, 5 Feb 2026: the tax authority integrated its national product catalog with the platform. This created **200.8k detailed MXIK identification codes with units of measure** for construction materials. The generalized MXIK may be used January–June 2026; "2026 yil 1 iyuldan boshlab … umumiy MXIKni qoʻllash bosqichma-bosqich cheklanadi". The classifier is accessed at mkinfo.uz/classifiers. New-code requests go through the mkinfo.uz personal cabinet with ERI. Support: t.me/mkinfo_support, +998 71 210-11-04 — [buxgalter.uz](https://buxgalter.uz/oz/publish/doc/text213447_2026_yilda_qurilish_materiallari_uchun_qanday_mhikdan_foydalanamiz); see also [tasnif.soliq.uz/news-view/5063](https://tasnif.soliq.uz/news-view/5063) (not fetched)
- TMSITI (Technical Regulation and Standardization Research Institute) is the institute behind the classifier and standards. Its site hosts ШНК PDFs — [tmsiti.uz/about](https://tmsiti.uz/about)

**Normative estimate base (resource codes)**
- ШНК 4.02.xx are Uzbek "элементные сметные нормы" (element estimate norms, resource method). Example: ШНК 4.02.41-06 is hosted at [tmsiti.uz PDF](https://www.tmsiti.uz/pdf/FILES/shnk/%D0%A8%D0%9D%D0%9A_4.02.41-06.pdf). ШНК 4.02.15-04 (finishing works) and ШНК 4.02.46-04 (reconstruction works) define labour, machine and material needs for resource-method estimates (search summary) — [mc.uz PDF](https://mc.uz/uploads/mcuz_98422534357045.pdf)
- Goskomarkhitektstroy issued methodological recommendations on a single coding methodology for resources in pricing norms, and ШНК 4.03.01-04 (machine operation) (search summary) — [mc.uz PDF](https://mc.uz/uploads/mcuz_82677385816528.pdf)
- Legacy: Cabinet decree **No. 463 of 24.10.2003** told the Ministry of Construction to create an information data bank of prices for construction materials, structures, machine operation, transport and builders' wages by region — [lex.uz/docs/246648](https://lex.uz/docs/246648?ONDATE=16.08.2024). Cabinet decree **No. 306 of 17.06.1997** moved estimating to the resource method on 1991 price-level norms — [nrm.uz](https://nrm.uz/contentf?doc=331021_postanovlenie_kabineta_ministrov_respubliki_uzbekistan_ot_17_06_1997_g_n_306_o_perehode_k_opredeleniyu_stoimosti_stroitelstva_na_osnove_resursnogo_metoda_na_baze_novyh_smetnyh_normativov_urovnya_cen_1991_goda&products=1_vse_zakonodatelstvo_uzbekistana)
- Ministry documents from 2022 refer to quarterly regional "current price catalogs" for construction materials (search summary; not verified) — [orginfouz PDF](https://orginfouz.s3.amazonaws.com/media/files/2022/8/19/202208194740388903.PDF)
- The 2024 Cabinet decree on the Ministry's structure (No. 214, 17.04.2024) is on lex.uz (not fetched) — [lex.uz/docs/-6887886](https://lex.uz/docs/-6887886)

### Inferences
- **App use (catalog backbone, UZ):** model the app's Uzbek material catalog on the national classifier plus MXIK. Store `uz_classifier_code`, `mxik_code`, name (uz/ru), unit, and GOST/standard. These are the identifiers that state estimates, e-invoices and supplier price lists will increasingly share after the July-2026 MXIK detailing, so matching user and supplier price lists to them is the most durable join key.
- **App use (prices, UZ):** mkinfo.uz is the closest thing to an "official current price" source. It has no documented API and is funded by user fees, so (a) do not scrape the internal `mk.shaffofqurilish.uz/api`; (b) approach TMSITI and the new pricing center about a data-sharing or partner agreement; (c) until then, seed default prices manually from public sources (UZEX cement quotes, retail listings) and let users override them.
- Official legal acts on lex.uz are normally not copyright-protected as official documents. That is a general legal principle I did not verify for Uzbekistan specifically. It still does not make the platform's *database* open data.
- In Uzbekistan "текущие цены" in estimates now effectively means "platform price at the estimate date." A consumer or small-contractor app does not need legal-grade prices, but it should label default prices as "indicative."

### Gaps
- I could not confirm whether mkinfo.uz shows prices publicly without login, how often prices refresh, or the exact code format (digit mask) of the national classifier. The SPA could not be rendered and API probing was not attempted.
- I found no 2025–2026 Ministry of Construction "ресурсные цены / текущие цены" PDF or Excel bulletin, and no "Республиканский центр ценообразования в строительстве" bulletin. These appear to have been replaced by the platform, but that is unconfirmed.
- The number, date and start date of the June 2026 decree creating the new pricing center were not found (lex.uz not searched for it).
- I found no evidence of a "СРН 4.02" naming; the Uzbek series found is ШНК 4.02.xx (resource/element norms).
- gov.uz/mc pages were not fetched.

---

## Q2. Uzbekistan — Uzbek Republican Commodity Exchange (UZEX, uzex.uz)

### Takeaway
UZEX is a real price-discovery source for **cement** (and other bulk goods). It publishes weekly digests and a "Haftalik kotirovkalar" (weekly quotations) table with PDF export. It has no Excel or open API, and archive/monitoring data requires membership. Its quotes are fed automatically into the national classifier platform by decree, so the app can get exchange prices indirectly.

### Cited Findings
- Weekly digest for 8–14 Sep 2025: **cement** 415,797.0 t offered, 101,900.0 t sold (25%). Average daily price fell from 672.8k to **645.4k UZS per tonne**. 76 average daily buyers. No rebar, timber, bitumen or glass in that digest — [uzex.uz digest](https://uzex.uz/en/News/uzex-weekly-digest-september-8-14)
- Weekly digest for 5–11 Jan 2026: 16,875 deals across all trading platforms (search summary) — [uzex.uz digest](https://uzex.uz/News/uzex-weekly-digest-january-5-11)
- 2024: cement exchange sales rose ~58% to ~5.9 Mt, and the 2024 average was about 534,700 UZS/t (search summary) — [globalcement.com](https://globalcement.com/news/18919-uzbekistan-s-cement-sales-rose-by-58-in-2024); [uzdaily.ru](https://uzdaily.ru/en/cement-sales-on-uzex-increase-by-58-in-2024/). 2023: cement prices fell from ~672,300 to ~583,500 UZS/t late in the year, and exchange cement sales fell 43.5% (search summary) — [daryo.uz](https://daryo.uz/en/2024/01/11/uzbekistan-records-435-decline-in-cement-exchange-sales-for-2023)
- An AzerNews headline says construction materials were the most traded goods on UZEX (not fetched; headline only) — [azernews.az](https://www.azernews.az/region/140545.html)
- **Weekly quotations page** — [uzex.uz/WeeklyQuotes](https://uzex.uz/WeeklyQuotes):
  - Each row has commodity, current price, absolute change, % change, and week start/end.
  - A dropdown selects weekly periods (latest seen: **26.10.2026–30.10.2026**), with "Внутренние" (domestic) and "Экспортные" (export) variants, selected by `importId`.
  - Paging is 10–1000 rows; about 1,327 records.
  - **PDF export** at `/WeeklyQuotes/Pdf?importId=102`; no Excel. The visible rows were fuels.
- **"Price level information"** page ([uzex.uz/Home/Monitoring](https://uzex.uz/Home/Monitoring)) filters by contract number, date range and min/max price. It says "Arxiv ma`lumotlarini olish uchun saytga a`zo bo`ling" (become a member to get archive data).
- **Commodities quotes hub** ([uzex.uz/en/pages/commodities-quotes](https://uzex.uz/en/pages/commodities-quotes)) links to quotation lists by currency (sum, USD, RUB, EUR, CNY; e.g. `/Trade/ContractsSumNew`), weekly quotes, clearing and trade registers. It has PDF notices of average prices (gold, wheat). There is a broker/customer personal cabinet, registration at spot.uzex.uz, and no API mention.
- Decree 231 Annex 4, stage 3: UZEX weekly construction-resource quotes are mirrored automatically on the national classifier platform — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291)

### Inferences
- **App use:** use UZEX as a *reference/benchmark* for bulk materials (cement especially; check the weekly table for metal products and others). Ingest manually or semi-automatically from the weekly PDF or digest into an internal "reference price" table with date and source. Do not present exchange prices as retail: they are ex-works or wholesale, per tonne, without delivery or retail margin. Show "exchange price + typical markup" or keep it as a hint.
- UZEX terms of reuse were not found. Treat the data as "cite-with-attribution, no bulk redistribution" until clarified.

### Gaps
- I could not confirm from fetched pages which construction materials beyond cement trade on UZEX in 2026 (rebar, rolled metal, timber, bitumen, glass, slate). The fetched pages did not list them.
- No UZEX API, open-data feed or terms of use was found.

---

## Q3. Uzbekistan — statistics agency (stat.uz / siat.stat.uz) and open data (data.gov.uz)

### Takeaway
SIAT (siat.stat.uz) provides **openly licensed (CC BY 4.0)** construction price *indices* with CSV, JSON, XML and XLSX downloads and an SDMX-style API endpoint. It does not provide per-material prices. Detailed building-material producer price indices appear to be sold as paid collections. data.gov.uz has no clear construction-material price dataset.

### Cited Findings
- SIAT dataset **1302**, "Dynamics of the price index for investments in fixed assets and construction works (to December of the previous year)", code 1.11.02.0012, Department of Price Statistics — [siat.stat.uz/data/1302](https://siat.stat.uz/data/1302/?lang=en):
  - Values: 2025 = **103.1**, 2024 = 106.5, 2023 = 108.1, 2022 = 110.3 (peak 136.0 in 2017). Annual; last updated **2026-01-16**.
  - Downloads in **CSV, JSON, XML, XLSX, PDF** through `http://api.siat.stat.uz/sdmx/1302/table/download/?download_format=...`. Licence **CC BY 4.0**, attribution to the Statistics Agency.
  - Methodology uses monthly prices for purchased building materials, parts and structures, construction wages, producer price indices and the USD rate.
- The stat.uz price statistics section mainly covers CPI and housing price indices — [stat.uz prices-and-indexes](https://stat.uz/ru/ofitsialnaya-statistika/prices-and-indexes). 2026 CPI: Jan–Mar 1.9%, Jan–May 2.7%, Jan–Jun 3.3% (search summary) — [stat.uz PDF Jan–Jun 2026](https://stat.uz/img/news/talil-2026-j_-yanvar-iyun-ruscha-_p99089.pdf)
- A stat.uz press release refers to a collection with "ayrim qurilish materiallari ishlab chiqaruvchilarning narx indekslari" (price indices for some construction material producers), shown as a **paid collection** ("To'plamni xarid qilish") (search summary) — [stat.uz construction statistics collection](https://stat.uz/uz/matbuot-markazi/qo-mita-yangiliklar/10269-o-zbekistonda-qurilish-statistik-to-plami-6)
- stat.uz has a news item "Qaysi qurilish materiali narxida eng yuqori o'sish kuzatildi" (which construction material price rose most). It returned HTTP 403 to my fetcher — [stat.uz](https://stat.uz/ru/press-tsentr/novosti-goskomstata/31036-qaysi-qurilish-materiali-narxida-eng-yuqori-o-sish-kuzatildi-2)
- The stat.uz open-data construction category includes "Qurilish ishlari hajmi (yillik)" (annual construction works volume) in xlsx, dated 13/11/2025. No per-material price dataset was found on data.gov.uz. An older Ministry of Construction dataset on olddata.gov.uz (2021) has unclear content (search summary) — [stat.uz construction](https://stat.uz/uz/rasmiy-statistika/construction-2); [olddata.gov.uz/en/datasets/16629](https://olddata.gov.uz/en/datasets/16629)
- Decree 231 stage 8: the Statistics Committee supplies names, units and price indicators from the producer price report (form 12) to the classifier platform — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291)
- Secondary: Realting.uz, citing Goskomstat, says construction material prices rose 12–18% in H1 2025, with cement and metal leading. This is an aggregator source, not verified against stat.uz — [realting.uz](https://realting.uz/news/how-does-inflation-affect-construction)

### Inferences
- **App use:** SIAT construction indices are the one *legally reusable* Uzbek official price series (CC BY 4.0). Use them to (a) age the static default catalog ("prices as of date X, indexed by SIAT construction index") and (b) show inflation hints. Poll the SDMX download endpoint after each annual update in mid-January, and check for monthly or quarterly series in the SIAT catalog (siat.stat.uz/data/).
- The mkinfo frontend route `/estat-sdmx-data` suggests the classifier platform already pulls SIAT SDMX data. This is consistent with decree 231, but it is an inference from route names only.

### Gaps
- I did not find a monthly SIAT series of building-material prices or indices by material (cement, brick, rebar). The SIAT catalog was not exhaustively browsed.
- Exact data.gov.uz search results were not retrieved; no construction-price dataset is confirmed.

---

## Q4. Uzbekistan — marketplaces, aggregators, suppliers and bazaars

### Takeaway
Retail and wholesale prices exist online in large volume (Prom.uz, Glotr.uz, OLX.uz, Uzum), but **none offers an official read API for third-party price aggregation**. OLX's Partner API does not cover Uzbekistan, and Uzum's seller API only exposes the seller's own shop. Building-material bazaars (Jomiy, Sergeli) do not publish structured price lists online. Supplier price lists will have to come from suppliers themselves, mostly as Excel or Telegram messages.

### Cited Findings
- **OLX.uz:** OLX Group developer terms list Partner API agreements for Poland, Bulgaria, Romania, Portugal, Ukraine and Kazakhstan, **not Uzbekistan** — [developer.olxgroup.com terms](https://developer.olxgroup.com/terms-and-conditions). A community MCP server for olx.uz says the official Partner API is not open for Uzbekistan and works through session cookies and internal APIs (unofficial) — [glama.ai olx-mcp-server](https://glama.ai/mcp/servers/lxz-401/olx-mcp-server). Third-party Apify scrapers cover olx.uz — [apify.com olx-scraper](https://apify.com/clover_engineer/olx-scraper)
- **Uzum Market:** seller OpenAPI at `https://api-seller.uzum.uz/api/seller-openapi`. The Swagger reference is gated behind the seller cabinet, and the token comes from the seller cabinet's API section. It covers only the seller's own products and orders (third-party connector docs, not official) — [apps.odoo.com mp_uzum_uz](https://apps.odoo.com/apps/modules/19.0/mp_uzum_uz); [anythingmcp.com Uzum guide](https://anythingmcp.com/guides/connect-uzum-market-to-claude)
- **Prom.uz:** the "Строительные материалы" section has ~13,800 products and "Кровельные материалы" ~2,000. Cards include price, description, supplier address and phone (search summary) — [prom.uz/section/stroitelnye-materialy](https://www.prom.uz/section/stroitelnye-materialy/)
- **Glotr.uz:** a catalog of ~18–19k building-material offers in Tashkent. Example seller card: roofing bitumen 4,500 so'm and galvanized sheet 200,000 so'm; dates unknown (search summary) — [glotr.uz/stroymateriali-v-tashkente](https://glotr.uz/stroymateriali-v-tashkente/)
- **Top.uz:** a directory of 231 building-material wholesalers in Tashkent, with no prices — [top.uz stroymaterialy-optom](https://top.uz/section/stroymaterialy-optom)
- **StroyVitrina.uz:** has a publication "Стоимость наиболее востребованных строительных товаров" (not fetched) — [stroyvitrina.uz](https://stroyvitrina.uz/publication/280-stoimost-naibolee-vostrebovannyh-stroitel-nyh-tovarov)
- Decree 231 stage 5 lets businesses post price information on the national classifier platform with ERI. The mkinfo frontend has routes for supplier "material ads", "offers" and "requests" — [lex.uz/docs/-6896291](https://lex.uz/docs/-6896291); own inspection of mkinfo.uz bundle

### Inferences
- **App use:** do not build on scraping OLX, Prom or Glotr. Without a partner API, scraping conflicts with typical marketplace ToS and is fragile. Instead:
  - (1) Let users import supplier price lists (Excel/CSV, photo or PDF with OCR, pasted Telegram text).
  - (2) Run a "supplier portal": suppliers upload their price list or connect an Excel or Google Sheet URL.
  - (3) Optionally deep-link to OLX, Prom or Uzum searches for a material name, without storing their prices.
- Bazaar and dealer prices in Tashkent (Jomiy, Sergeli, Yangiobod and others) are mostly shared through Telegram channels and phone. A crowd-sourced "user-reported price" feature (price, unit, place, date, photo of price tag) is a realistic way to collect them.

### Gaps
- stroyka.uz, «Qurilish mollari bozori», Jomiy and Sergeli bazaars: no online price lists or official sites were found.
- I did not find the ToS text of OLX.uz, Prom.uz or Glotr.uz on scraping. Any reuse must be checked against each ToS.
- Large Uzbek retail chains with online price lists (e.g., home-improvement chains) were not identified in this research.

---

## Q5. Russia — ФГИС ЦС (fgiscs.minstroyrf.ru): КСР codes, quarterly price monitoring, downloads, API

### Takeaway
ФГИС ЦС is Russia's authoritative source. It holds the **КСР** classifier (hierarchical codes, updated several times a year; latest revision by Minstroy order **13.02.2026 № 82/пр**) and **quarterly regional сметные цены** (136,021 prices for Q1 2026). Files are downloadable (Excel and "сплит-формы" that GRAND-Smeta imports). I found **no public API or formal open-data licence**. Quarterly publication in 2026 came around 25 Feb (Q1), 22 May (Q2) and 24 Aug (Q3); Q4 2026 was probably not yet out on 2026-11-06.

### Cited Findings
- **Q1 2026:** "сметные цены на материалы, изделия, конструкции, оборудование и эксплуатацию машин и механизмов на I квартал 2026 года в количестве **136 021** цены", +26.5% vs Q4 2025. Published 25 Feb 2026 on the basis of Minstroy letter 25.02.2026 № 9859-ИФ/09 (search summary) — [digital-build.ru](https://digital-build.ru/news/napolnyaemost-fgis-czs-dannymi-o-fakticheskoj-stoimosti-stroitelnyh-resursov-vyrosla-na-265/); [jcement.ru](https://jcement.ru/content/news/napolnyaemost-fgis-tss-dannymi-o-fakticheskoy-stoimosti-stroitelnykh-resursov-uvelichena-na-26-5/); [normacs.info](https://www.normacs.info/news/78460)
- Split-forms with resource prices and indices for Q1 2026 were published on 25 Feb 2026 for Russian regions and industry companies — [ekspbur.ru](https://ekspbur.ru/news/26824/); [zsccs.ru](https://zsccs.ru/news/2026-02-25/25-fevralya-2026-goda)
- **Q2 2026:** indices to groups of homogeneous resources were approved by letter 22.05.2026 № 31076-ИФ/09 for 85 regions plus ЗАТО Саров (search summary) — [its.i-tat.ru Q2 2026](https://its.i-tat.ru/articles/vyshla-split-forma-za-2-kvartal-2026-g)
- **Q3 2026:** indices by letter 24.08.2026 № 52419-ИФ/09 (search summary) — [ergro.ru](https://ergro.ru/programmy/dlya-smetchika/indeksy-koeffitsienty-perescheta/indeksy-izmeneniya-smetnoy-stoimosti-minstroya/); [its.i-tat.ru/indexes](https://its.i-tat.ru/indexes/). For Nizhny Novgorod oblast, ФГИС ЦС published Q3 2026 сметные цены of construction resources, transport and loading, and labour, plus indices to ГОСР for the resource-index method (РИМ). Section: "Сметные цены и индексы изменения сметной стоимости строительства" at [fgiscs.minstroyrf.ru/prices](https://fgiscs.minstroyrf.ru/prices) — [gradrazvitie.ru](https://gradrazvitie.ru/opublikovany-indeksy-k-gosr-i-smetnye-ceny-na-iii-kvartal-2026-goda-primenyaemye-pri-rim/)
- Publication rule (2018 article): prices for materials, products, structures and equipment are published "не позднее 25-го числа второго месяца квартала, следующего за отчетным". Machines and labour were annual from 2018 at that time — [isguru.ru (2018)](https://isguru.ru/bez-rubriki/6351-fgis-czs-voprosy-i-otvety-2/)
- Data suppliers: manufacturers and importers submit data through a personal cabinet with a qualified e-signature (УКЭП) and (in 2018) Jinn-Client and Kontinent TLS VPN. Non-submission can bring administrative liability. In 2018 there were 13,937 registered entities (6,849 manufacturers, 6,982 importers). For budget projects, estimates must use FGIS-registered resource codes or the expert review is negative — [isguru.ru (2018)](https://isguru.ru/bez-rubriki/6351-fgis-czs-voprosy-i-otvety-2/)
- **Downloads:** GRAND-Smeta 2023.1 added loading current prices and indices from the split-form and **from Excel files downloaded from the ФГИС ЦС site** — [cad.ru PDF](https://cad.ru/upload/iblock/6e0/nzobzr5e5oge15cw4addqq06z9n963lg.pdf). Minstroy order 24.10.2017 № 1470/пр (federal register of estimate norms) refers to publication in Excel format suitable for saving and text search — [rulaws.ru](https://rulaws.ru/acts/Prikaz-Minstroya-Rossii-ot-24.10.2017-N-1470_pr/)
- **КСР structure:** hierarchical classification with sequential coding. Materials, products, structures and equipment use **книга (XX) → часть (XX.X) → раздел (XX.X.XX) → группа (XX.X.XX.XX) → позиция (XX.X.XX.XX-XXXX)**, with an optional extra 3-digit suffix. Machines have no "часть" level. Up to 99 books, organized by ОКПД2 and GESN logic (search summary of expertsmet and others) — [expertsmet.ru](https://expertsmet.ru/articles/klassifikator-stroitelnyh-materialov-gruppy-i-kody-ksr/); [isguru.ru](https://isguru.ru/bez-rubriki/6351-fgis-czs-voprosy-i-otvety-2/). In 2018 КСР had 99,545 positions — [isguru.ru](https://isguru.ru/bez-rubriki/6351-fgis-czs-voprosy-i-otvety-2/). The official 2016 КСР form (Book 01 "Материалы для строительных и дорожных работ") is at [files.stroyinf.ru PDF](https://files.stroyinf.ru/Data2/1/4293744/4293744976.pdf)
  - Conflict: texoboronexpert describes the hierarchy as "раздел → группа → вид ресурса → порядковый номер". That differs from the книга/часть/раздел/группа description above; trust the official КСР on the portal — [texoboronexpert.ru](https://texoboronexpert.ru/novosti-stati/fgis-cs-klassifikator-stroitelnyh-resursov/)
- **КСР 2026 status:** updated by Minstroy order **13.02.2026 № 82/пр** ("очередная редакция вступила в силу с 13 февраля 2026 года"). The base order is 17.11.2022 № 969/пр, and order 02.03.2017 № 597/пр has lapsed. Changes have come several times a year since 2022. The portal operator is Главгосэкспертиза — [texoboronexpert.ru](https://texoboronexpert.ru/novosti-stati/fgis-cs-klassifikator-stroitelnyh-resursov/)
- Portal pages: КСР FAQ [fgiscs.minstroyrf.ru/faq?category=18](https://fgiscs.minstroyrf.ru/faq?category=18); "Сводный перечень строительных ресурсов" [fgiscs.minstroyrf.ru/ksr/sncr](https://fgiscs.minstroyrf.ru/ksr/sncr); КСР archive (materials, 2020) [fgiscs.minstroyrf.ru/ksr/archives/index/82](https://fgiscs.minstroyrf.ru/ksr/archives/index/82). The FAQ page rendered empty to my fetcher (SPA).
- Third-party mirrors: SmetaCheck has "Сметные цены на строительные материалы" — [smetacheck.ru/materials](https://smetacheck.ru/materials)
- **API / open data:** no official ФГИС ЦС API documentation was found. An older ФГИС ТП article says the operator must publish open data on resource costs for all participants, but this may be outdated — [fgis-tp.ru](https://fgis-tp.ru/smetnyx-cen-stroitelnyx-resursov-v-fgis-cs/)

### Inferences
- **App use (catalog backbone, RU):** use КСР codes as the primary key for Russian materials (`ksr_code`, name, unit, characteristics). Import the КСР list from the ФГИС ЦС downloads once, then refresh after each Minstroy update order (several times a year). Keep a code-mapping table because codes get split or retired, as the 2026 revision shows.
- **App use (prices, RU):** run a quarterly ETL job (late Feb, May, Aug, Nov). Download the regional Excel or split-form files for the regions the app supports, map by КСР code, and store `price_type="smetnaya_FGIS"`, region, quarter and VAT flag. These are *estimate* prices (без НДС, usually with delivery to site per methodology), not retail shelf prices. Label them accordingly.
- Licensing: official documents and state information are generally not copyright objects in Russia (ГК РФ ст. 1259 п. 6). That is general legal knowledge, not verified in this research, and the database or portal terms of use were not found. Reuse with attribution is likely acceptable, but get a legal review before bulk redistribution. There is no API, so ingestion means automated file downloads; check the portal's robots and terms.

### Gaps
- Exact file formats per region (xlsx vs xml), whether download needs login, and the portal's terms of use were not confirmed (SPA pages could not be read).
- Q4 2026 publication status: not found. It is probably due around late November 2026, based on the 2026 pattern.
- The precise official КСР code-mask rules (order 969/пр annex) were not read directly.

---

## Q6. Russia — regional price centers (РЦЦС), «Сборники текущих цен» (ТСЦ/ТСН), Minstroy indices

### Takeaway
Since the shift to ФГИС ЦС sметные цены and the resource-index method, Minstroy publishes **quarterly indices by letter** (e.g., 22.05.2026 № 31076-ИФ/09; 24.08.2026 № 52419-ИФ/09). Regional price centers mostly republish or extend this with split-forms and their own paid collections. I found no specific 2026 subscription prices or open formats for regional ТСЦ/ТСН collections.

### Cited Findings
- Minstroy quarterly index letters in 2026: Q1 25.02.2026 № 9859-ИФ/09; Q2 22.05.2026 № 31076-ИФ/09; Q3 24.08.2026 № 52419-ИФ/09 (search summaries) — [its.i-tat.ru Q1](https://its.i-tat.ru/articles/vyshla-split-forma-za-1-kvartal-2026-g); [its.i-tat.ru Q2](https://its.i-tat.ru/articles/vyshla-split-forma-za-2-kvartal-2026-g); [ergro.ru](https://ergro.ru/programmy/dlya-smetchika/indeksy-koeffitsienty-perescheta/indeksy-izmeneniya-smetnoy-stoimosti-minstroya/)
- Regional centers publish split-forms with resource prices and indices by region (e.g., ЗСЦЦС on 25 Feb 2026) — [zsccs.ru](https://zsccs.ru/news/2026-02-25/25-fevralya-2026-goda); [ekspbur.ru](https://ekspbur.ru/news/26824/)
- Moscow ТСН-2001 general guidance (Chapter 12) governs "конъюнктурный анализ" (market price analysis) in Smeta.ru. It requires at least three supplier offers (price lists, invoices, commercial proposals) — [cloud.smeta.ru 11.15 upgrade PDF](https://cloud.smeta.ru/upload/11.15.0.1/upgrade.pdf); [help11.smeta.ru what_news11_15](https://help11.smeta.ru/what_news11_15/what_news11_15.pdf). Market-analysis output is formatted per Minstroy order № 421/пр — [smetchik.com](https://mail.smetchik.com/services/smetnyy-otdel/konnkturnyy-analiz)
- An example of regional activity: a 26 Feb 2026 seminar on pricing by СРО «Строители Башкирии» and Минстрой РБ (search summary) — [forum.smeta.ru](https://forum.smeta.ru/download/file.php?id=29784)

### Inferences
- **App use:** for a consumer or contractor app, Minstroy indices are useful only to "age" an older base price list to the current quarter (price × index). Regional ТСЦ collections are paid, licensed products that estimating-software vendors buy. They are not a practical free source for an app.
- The "≥3 supplier offers" rule is a good UX pattern: let users attach several supplier quotes per material and auto-compute min, median or average. This mirrors official конъюнктурный анализ.

### Gaps
- Specific РЦЦС names, URLs, subscription prices and formats for 2026 were not found.
- Rosstat building-material price series (e.g., producer prices for cement, brick, rebar) were not researched in depth.

---

## Q7. Russia — retailers and marketplaces with structured data (Петрович, Lemana PRO, ВсеИнструменты, OBI, Yandex Market, Avito)

### Takeaway
None of the big Russian DIY or building retailers offers a **public read API of their catalog and prices** for third parties. Their APIs are **seller-side**: Lemana PRO marketplace API keys come through support; ВсеИнструменты suppliers use EDI (PRICAT) and a B2B cabinet. Yandex Market's Partner API returns only the caller's own offers. Avito's API is for managing your own ads. Retail prices for an app would need partnerships or affiliate feeds, not scraping.

### Cited Findings
- **Lemana PRO (ex-Леруа Мерлен):** sellers integrate through the marketplace API, and the x-api-key is issued by Lemana PRO support. Some functions (labels, cancel status) do not sync — [selsup.ru Lemana PRO](https://selsup.ru/help/integratsiya-s-lerua-merlen/). МойСклад describes automatic exchange of stock and prices by API for the FBO model only — [moysklad.ru](https://www.moysklad.ru/poleznoe/marketplejsy/kak-prodavat-na-lerua-merlen/amp/)
- **ВсеИнструменты:** "Открытого API для поставщика у площадки нет". Exchange is EDI via an operator with messages **PRICAT, ORDERS, ORDRSP, DESADV** and УПД. Third-party "API integrations" pull retail, wholesale and recommended prices, warehouse stock and transits from the supplier's B2B cabinet — [selsup.ru ВсеИнструменты](https://selsup.ru/blog/vseinstrumenti-api-integraciya/)
- **Петрович:** no public API documentation found. Supplier-integration capability "depends on access and the Petrovich supplier API" — [selsup.ru Петрович](https://selsup.ru/store/integratsiya-s-petrovich-postavschiki/). The Petrovich app lists more than 63,000 products — [App Store](https://apps.apple.com/us/app/%D0%BF%D0%B5%D1%82%D1%80%D0%BE%D0%B2%D0%B8%D1%87-%D1%80%D0%B5%D0%BC%D0%BE%D0%BD%D1%82-%D0%B4%D0%BE%D0%BC%D0%B0-%D0%BA%D0%B2%D0%B0%D1%80%D1%82%D0%B8%D1%80/id1246154462?l=ru). A 2018 case study describes Petrovich's B2B cabinet for construction companies — [korusconsulting.ru PDF](https://korusconsulting.ru/upload/generated_pdf/petrovich-avtomatizatsiya-raboty-s-b2b-klientami-kak-klyuch-k-zavoevaniyu-rynka-diy.pdf)
- **Yandex Market Partner API:** host `api.partner.market.yandex.ru`. The API-Key is created in the partner cabinet (Настройки → Доступ к API). Prices are read via `POST v2/campaigns/{campaignId}/offer-prices` (getPrices is deprecated) or `POST v2/businesses/{businessId}/offer-mappings`, scope `pricing:read-only`. This covers the seller's own offers only — [yandex.com partner-api getPrices](https://yandex.com/dev/market/partner-api/doc/ru/reference/prices/getPrices.md); [getPricesByOfferIds](https://yandex.com/dev/market/partner-api/doc/ru/reference/prices/getPricesByOfferIds.md)
- **Avito:** API base `https://api.avito.ru` with OAuth client credentials. Production and sandbox paths are under `/ads/` and `/ads-sandbox/` (third-party SDKs) — [pypi avito-py](https://pypi.org/project/avito-py/2.1.0.2/); [packagist avito-ads-sdk-php](https://packagist.org/packages/avito-tech/avito-ads-sdk-php). Scraping profiles via the API led to IP restrictions and a captcha (forum report) — [qna.habr.com](https://qna.habr.com/q/1352632). No confirmation that the API returns other sellers' listings.

### Inferences
- **App use (RU retail prices):** (1) Use ФГИС ЦС for official estimate prices. (2) For retail, pursue affiliate or partner programs (e.g., marketplace affiliate feeds) or supplier-provided YML or Excel price lists, rather than scraping Petrovich or Lemana. (3) Support user-entered prices with "where bought" and date. Retail prices change daily and are regional per store; cache them with a TTL and source label.

### Gaps
- OBI Russia's 2026 status and any data feeds: not researched (no results).
- Official Avito developer documentation (developers.avito.ru) and Petrovich, Lemana or ВсеИнструменты partner and affiliate programs were not fetched directly.

---

## Q8. Standards for exchanging price lists (Excel, CommerceML, YML, EDI, GAEB) — what suppliers actually send

### Takeaway
In both countries the practical norm is **Excel/CSV price lists exported from 1C**. CommerceML (1C's XML) and YML (Yandex's XML feed) are the main structured formats in Russia, and EDI PRICAT appears with large retailers. **GAEB is a German/EU bill-of-quantities standard** that Uzbek and Russian suppliers do not use. In Uzbekistan the emerging "standard" key is the **MXIK code plus the national classifier code** rather than a file format.

### Cited Findings
- **CommerceML:** a single XML standard for exchanging commercial information. First edition in 2000 (1C and Extra.RU). It covers the classifier, catalog, commercial offers (prices, stock) and documents. Versions 2.04 and 2.07 are referenced in practice — [ru.ruwiki.ru CommerceML](https://ru.ruwiki.ru/wiki/CommerceML); [its.1c.ru supplier–consumer exchange](https://its.1c.ru/db/content/metod8dev/src/developers/dataexchange/supplier%20-%20consumer/i8103433.htm); [infostart.ru](https://infostart.ru/public/512421/)
- Common 1C practice: scheduled export of the price list to a file (Excel) and copy by FTP — [qna.habr.com](https://qna.habr.com/q/205869). Loading supplier Excel price lists into 1C is a common Infostart tool — [infostart.ru](https://infostart.ru/public/147136/)
- **YML:** Yandex Market uses YML rather than CommerceML for product feeds (forum statement) — [qna.habr.com](https://qna.habr.com/q/34857). The official YML spec page was not retrieved; the URL I tried redirected to the general seller help — [yandex.ru/support/partnermarket](https://yandex.ru/support/partnermarket/export/yml.html)
- **EDI PRICAT:** ВсеИнструменты suppliers exchange price catalogs via EDI PRICAT — [selsup.ru](https://selsup.ru/blog/vseinstrumenti-api-integraciya/)
- **GAEB DA XML:** a German construction data exchange standard. X83 is the tender bill of quantities and X84 the priced bid. Current DA XML versions are 3.2 and 3.3, and open-source parsers include pyGAEB. Sources disagree on the full X80–X86 phase mapping — [pygaeb docs](https://pygaeb.readthedocs.io/en/latest/guides/parsing/); [streit-software.de](https://www.streit-software.de/wissen/gaeb-formate)
- Uzbek MXIK detailing (200.8k construction codes with units) applies from 1 July 2026, and MXIK codes appear on e-invoices — [buxgalter.uz](https://buxgalter.uz/oz/publish/doc/text213447_2026_yilda_qurilish_materiallari_uchun_qanday_mhikdan_foydalanamiz)

### Inferences
- **App use:** build one flexible **price-list importer** in this order:
  1. **Excel/CSV** with column mapping (name, unit, price, currency, VAT, optional code — MXIK, КСР or vendor SKU, date). Save mapping templates per supplier.
  2. **YML** (common for Russian e-shops; easy XML).
  3. **CommerceML** `offers.xml` and `import.xml` (1C).
  4. Optional OCR of PDF or photo price lists.

  Skip GAEB unless targeting EU users.
- Matching supplier rows to catalog items: exact code match first (MXIK or КСР), then a fuzzy name+unit match with user confirmation, then remember the mapping per supplier.

### Gaps
- No survey data on what share of Uzbek suppliers send Excel vs 1C vs Telegram. This is anecdotal and not sourced.
- The official YML specification and 1C's current CommerceML version (3.x?) were not verified.

---

## Q9. How existing estimating software handles prices (GRAND-Smeta, Smeta.ru, Uzbek software)

### Takeaway
Desktop estimating software uses a **three-layer model**: (1) a built-in normative base with base prices; (2) official current prices or indices loaded per region and quarter (Excel or split-form files from ФГИС ЦС in Russia); (3) user and supplier price lists through a "price monitoring" directory that averages several suppliers. Uzbek tools (АВС, smeta.uz) work on ШНК 4.02 resource norms, but their price-update mechanism is not documented publicly.

### Cited Findings
- **GRAND-Smeta 2023.1:** loads current prices and indices from the ФГИС ЦС split-form and Excel files. The user saves the Excel file locally and adds it to the list of documents available for loading into the estimate; one action loads both current prices and indices for the resource-index calculation — [cad.ru PDF](https://cad.ru/upload/iblock/6e0/nzobzr5e5oge15cw4addqq06z9n963lg.pdf)
- **Smeta.ru:** the "Мониторинг цен" directory manages suppliers and their price lists and shows the average price per material across suppliers' lists — [help.smeta.ru/20/20.pdf](https://help.smeta.ru/20/20.pdf). Smeta.ru 11.15 supports конъюнктурный анализ per ТСН-2001 (≥3 offers) and imports market-analysis data from Excel — [cloud.smeta.ru upgrade.pdf](https://cloud.smeta.ru/upload/11.15.0.1/upgrade.pdf)
- **АВС (abccenter.uz):**
  - Supports CIS methodologies, "различными валютами и масштабами цен" (different currencies and price scales), and all four pricing methods: basic-index, basic-compensation, resource and resource-index.
  - For the resource method, its ИНФО subsystem has a "механизм замены или удаления ресурсов (машин или материалов)".
  - Its "Экспертиза" tool compares the estimate's region with the normative data region.
  - No public description of price updates was found.
  — [abccenter.uz/abc](https://abccenter.uz/abc/)
- **smeta.uz** (copyright Soft Project Systems, contact info@tnq.uz) lists the normative base ШНК 4.02.00-04 (general construction), 4.17.00-05 (installation), commissioning, 4.02.70-05 (repair), aggregated resource norms and 4.04.06-14 (road transport). It has sections for machine operation, materials and equipment, plus "Дефектный акт", "Технический надзор" (login) and "Экспертиза сметы". No prices or dates were visible on the fetched page — [smeta.uz/basis/6/6](http://smeta.uz/basis/6/6)
- **USU "Программа для смет" (ususoft.uz):** a generic accounting-style tool. It claims "текущие цены", Excel/Word export and 1C integration; Uzbek and Russian UI; one-time payment. It does not document its price sources — [ususoft.uz](https://ususoft.uz/programma_dlya_sostavleniya_smety.php)
- A Goskomarkhitektstroy institute document mentions a computer program it developed to automate the process (name not given; search summary) — [mc.uz PDF](https://mc.uz/uploads/mcuz_93125933828970.pdf)

### Inferences
- **App use:** copy the three-layer model in a mobile-friendly form:
  - **Layer A, default catalog:** curated common materials with units, waste factors and indicative prices per region and date. Key on national codes (UZ classifier/MXIK, RU КСР).
  - **Layer B, official or reference updates:** UZ: UZEX benchmarks, SIAT indices, platform prices if a partnership happens. RU: ФГИС ЦС quarterly regional prices and Minstroy indices.
  - **Layer C, user and supplier price lists:** import, override and average several quotes.

  Price resolution order per estimate line: user override → selected supplier list → regional reference price → indexed default.
- Mobile "Строительный калькулятор"-type apps typically ship no live price database and rely on user-entered prices. This is an inference; not verified in this research.

### Gaps
- No Uzbek mobile estimating app with a live price feed was found. I could not confirm whether АВС or smeta.uz pull prices from mkinfo.uz.
- «Мастер Сметы» and mobile "Строительный калькулятор" apps were not researched (no sources gathered).

---

## Q10. Synthesis — practical data-source plan for the app (static catalog + updates + user/supplier price lists)

### Takeaway
Build the catalog on **official codes** (UZ: national classifier/MXIK; RU: КСР). Seed indicative prices from public, reusable sources (UZ: UZEX and SIAT CC-BY indices; RU: ФГИС ЦС quarterly regional files). Make **user and supplier price-list import the main live-price channel**, because no official or retail source in either country offers an open price API.

### Cited Findings
Source-by-source summary (facts as cited in the sections above):

| Source | Country | Contains | Update | Format / access | Reuse / licence | Reliability |
|---|---|---|---|---|---|---|
| Qurilish resurslari milliy klassifikatori — [mkinfo.uz](https://mkinfo.uz/) (ex-catalog.tmsiti.uz) | UZ | ~181.7k materials + machines, equipment and works (May 2024); codes, units, GOST; prices from manufacturers, UZEX, tax, customs, procurement, statistics | Continuous feeds; UZEX weekly | Web SPA; ERI login for suppliers; undocumented internal API | Funded by user fees (decree 231); no open licence found | Official; legally used for state estimate starting prices |
| MXIK detailed codes — [buxgalter.uz](https://buxgalter.uz/oz/publish/doc/text213447_2026_yilda_qurilish_materiallari_uchun_qanday_mhikdan_foydalanamiz), tasnif.soliq.uz | UZ | 200.8k construction-material codes with units | From 1 Jul 2026 (stepwise) | Web lookup | Official identifiers | High |
| UZEX — [uzex.uz/WeeklyQuotes](https://uzex.uz/WeeklyQuotes), digests | UZ | Exchange prices (cement confirmed; e.g. 645.4k UZS/t, Sep 2025) | Weekly | HTML table + PDF; archive needs membership | Terms not found | High for wholesale/ex-works |
| SIAT — [siat.stat.uz/data/1302](https://siat.stat.uz/data/1302/?lang=en) | UZ | Construction and investment price index (2025 = 103.1) | Annual (updated 2026-01-16) | CSV/JSON/XML/XLSX; SDMX download URL | **CC BY 4.0** | High (index only) |
| Prom.uz / Glotr.uz / OLX.uz / Uzum | UZ | Retail and wholesale listings with prices | Continuous | Web; OLX Partner API not for UZ; Uzum seller API (own shop only) | No third-party data rights | Variable, often stale |
| ФГИС ЦС — [fgiscs.minstroyrf.ru/prices](https://fgiscs.minstroyrf.ru/prices) | RU | 136,021 сметные цены (Q1 2026) by region; КСР classifier | Quarterly (~25 Feb, May, Aug, Nov); КСР several times a year | Excel and split-form downloads; no API found | State data; terms not found | Official |
| Minstroy indices (letters) | RU | Indices to ГОСР by region | Quarterly | PDF, Excel, split-form | Official docs | Official |
| Yandex Market Partner API, Lemana PRO, ВсеИнструменты EDI, Avito | RU | Seller's own offers and prices | Real-time | Seller APIs and EDI | Only for your own shop | n/a for aggregation |

- Contract-price overheads in UZ (other costs 20%, profit 7%, production costs 2.5%/5%) come from the Oct 2026 regulation — [spot.uz](https://www.spot.uz/oz/2026/10/23/contract-organizations)

### Inferences
- **Static default catalog (ship in app):** ~300–1,000 most-used materials per country.
  - Fields: internal id; `uz_classifier_code`/`mxik_code` or `ksr_code`; names in uz, uz-cyrl and ru; unit plus conversion units (e.g. a bag of cement is 50 kg, tonne vs m³); default waste %; `indicative_price`, `currency`, `region`, `price_date`, `price_source`.
  - Ship it as a versioned JSON or SQLite asset and update it OTA from the app's backend.
- **Updates (backend jobs):**
  - UZ: weekly manual or semi-automatic capture of UZEX quotes for bulk materials; annual SIAT index pull through the SDMX download (CC BY attribution in the app's "Sources" screen); pursue an official data-sharing agreement with TMSITI or the new «Shaharsozlik sohasida iqtisodiy islohotlar va narxlarni shakllantirish respublika markazi» for mkinfo prices.
  - RU: quarterly ФГИС ЦС regional Excel/split-form import keyed by КСР; refresh КСР after each Minstroy order.
- **User and supplier price lists (primary live channel):** an Excel/CSV/YML/CommerceML importer with column mapping and code or fuzzy matching. Add a supplier self-service upload (web) with "valid until" dates, and crowd-sourced user price reports (price, place, date, photo).
  - Resolution order per line: user override → chosen supplier → regional reference → indexed default.
  - Show the source and age of every price, and flag prices older than N days.
- **Avoid:** scraping OLX, Prom, Glotr, Petrovich or Lemana, and calling mkinfo's undocumented API, without written permission (ToS and legal risk, fragility).
- **Currency and VAT:** UZ prices are in UZS (UZEX per tonne; retail per piece or bag). RU ФГИС ЦС prices are estimate prices without VAT. Store `vat_included` and a `price_basis` (ex-works / delivered / retail) on every price record.

### Gaps
- Licensing for reuse of mkinfo.uz, UZEX and ФГИС ЦС data is not documented in any fetched source. Written confirmation from operators (TMSITI or the new pricing center, UZEX, Главгосэкспертиза/Минстрой) is needed before redistributing their prices inside a commercial app.
- I found no public evidence of any party offering an Uzbek construction-price API (official or commercial) as of Nov 2026.
