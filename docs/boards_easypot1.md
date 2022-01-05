#EasyPot1

A good starting point to make sure your pot won't suffer due to lack of attention :)

Goal of this board is to introduce you to GrowNode platform, using super common components you can buy online.

##Features

- Soil moisture level detection with configurable low/high treshold
- Light level detection 
- Temperature detection
- Automatic watering system (optional)
- Access to online tracking system to track your growing progress

The board is equipped with LED showing the need to water your plant, excess light and temperature out of target

##Bill of Material

- 1 capacitive moisture level sensor [aliexpress](https://it.aliexpress.com/item/32864255890.html?spm=a2g0o.search0302.0.0.5c99558eRVKgmF&algo_pvid=fe8b7ef4-2e3a-4970-850d-d82c474e6777&aem_p4p_detail=202201051246061694626298027740030537040&algo_exp_id=fe8b7ef4-2e3a-4970-850d-d82c474e6777-0R)
- 1 light sensor [aliexpress](https://it.aliexpress.com/item/33044711112.html?spm=a2g0o.productlist.0.0.212389ec3QaJls&algo_pvid=12576346-9834-4177-aa30-7fa1c6fdf754&aem_p4p_detail=202201051244519903133772029240030524842&algo_exp_id=12576346-9834-4177-aa30-7fa1c6fdf754-1&pdp_ext_f=%7B%22sku_id%22%3A%2267419412781%22%7D&pdp_pi=-1%3B1.41%3B-1%3BEUR+1.05%40salePrice%3BEUR%3Bsearch-mainSearch)
- 1 temperature sensor [aliexpress](https://it.aliexpress.com/item/1005001636433931.html?spm=a2g0o.productlist.0.0.2be22a659R9vD1&algo_pvid=29a85c9b-c44a-4d82-88d4-d77f482607d7&aem_p4p_detail=202201051248362525021342591050030527186&algo_exp_id=29a85c9b-c44a-4d82-88d4-d77f482607d7-5&pdp_ext_f=%7B%22sku_id%22%3A%2212000016918505158%22%7D&pdp_pi=-1%3B1.68%3B-1%3BEUR+1.29%40salePrice%3BEUR%3Bsearch-mainSearch)
- 1 peristaltic water pump [aliexpress](https://it.aliexpress.com/item/1005003166461443.html?spm=a2g0o.productlist.0.0.611617ae1vb4Wc&algo_pvid=522d00f9-a16e-44dc-9683-0fd4e52df36f&algo_exp_id=522d00f9-a16e-44dc-9683-0fd4e52df36f-50&pdp_ext_f=%7B%22sku_id%22%3A%2212000024450582578%22%7D&pdp_pi=-1%3B2.29%3B-1%3BEUR+2.97%40salePrice%3BEUR%3Bsearch-mainSearch)
- LED [aliexpress](https://it.aliexpress.com/item/32626322055.html?spm=a2g0o.productlist.0.0.637b3bcfbiyAa5&algo_pvid=4d6cd08f-f9b7-4ff3-9a05-e983cb1dd17c&aem_p4p_detail=202201051257527368520968769800030588268&algo_exp_id=4d6cd08f-f9b7-4ff3-9a05-e983cb1dd17c-35&pdp_ext_f=%7B%22sku_id%22%3A%2259399079352%22%7D&pdp_pi=-1%3B3.03%3B-1%3BEUR+0.96%40salePrice%3BEUR%3Bsearch-mainSearch)
- 220ohm resistors [aliexpress](https://it.aliexpress.com/item/1005002631550177.html?spm=a2g0o.productlist.0.0.20cdbd970OCIhw&algo_pvid=9eff731c-5630-4da2-8214-68107f77e5b0&aem_p4p_detail=202201051259481008733339598140030601318&algo_exp_id=9eff731c-5630-4da2-8214-68107f77e5b0-0&pdp_ext_f=%7B%22sku_id%22%3A%2212000021480015801%22%7D&pdp_pi=-1%3B1.9%3B-1%3BEUR+0.79%40salePrice%3BEUR%3Bsearch-mainSearch)
- 1 L298N motor shield [aliexpress](https://it.aliexpress.com/item/4000408025093.html?spm=a2g0o.productlist.0.0.4e3e69a47kGlnF&algo_pvid=3f89f4a9-ec5b-4ece-84b9-d05ba4a91301&aem_p4p_detail=202201051300511067026755126980030602174&algo_exp_id=3f89f4a9-ec5b-4ece-84b9-d05ba4a91301-2&pdp_ext_f=%7B%22sku_id%22%3A%2210000001683449944%22%7D&pdp_pi=-1%3B0.54%3B-1%3B-1%40salePrice%3BEUR%3Bsearch-mainSearch)

##Assembling

todo

##Code

The configuration code is contained in `components\grownode\boards\easypot1.c`.

##Controlling the pot

todo