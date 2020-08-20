# BotTender

BotTender è un cocktail maker fatto con arduino che attraverso delle pompe peristaltiche create con dei motori passo-passo nema 17 ti permette di scegliere un drink da una lista predefinita. 

La quantità versata viene scelta dall'utente in base a quanti cm di altezza vuole nel suo bicchiere, e il cocktail viene servito rispettando le proporzioni della ricetta.

## Materiale utilizzato
- Arduino uno
- Alimentatore 19V 4.7A
- 4x Stepper Motor nema 17 1.5A 
- 4x Modulo A4988 driver per moter passo-passo
- Condensatore 50V 100$\mu F$ 
- Rotatory encoder
- Display LCD 16x2 con interfaccia I2C
- Sensore di distanza ad ultrasuoni

## Librerie utilizzate
- LiquidCrystal_I2C