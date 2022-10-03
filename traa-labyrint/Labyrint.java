import java.util.Scanner;
import java.io.File;
import java.util.ArrayList;
import java.util.Set;
import java.io.FileNotFoundException;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class Labyrint{
  private ArrayList<Thread> aktiveTrader = new ArrayList<Thread>();
  private Rute[][] rutenett;
  private ArrayList<ArrayList<Rute>> utveier = new ArrayList<ArrayList<Rute>>();
  private JLabel antUtveierTekst;
  private ArrayList<Rute> likVei;

  //Sender inn fil som instansvariabel
  Labyrint(File fn) throws FileNotFoundException{
    this.rutenett = lagRutenett(fn);
  }

  //Viser Labyrinten grafisk med swing
  public void grafisk(){
    //Standard
    JFrame vindu = new JFrame("Quiz");
    vindu.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    JPanel panel = new JPanel(new BorderLayout());
    vindu.add(panel);

    //Info
    this.antUtveierTekst = new JLabel("ANTALL UTVEIER: 0" , SwingConstants.CENTER);
    panel.add(this.antUtveierTekst, BorderLayout.NORTH);

    /*
    //ALterntiv utforming
    JPanel infoPanel = new JPanel();
    infoPanel.setLayout(new BoxLayout(infoPanel, BoxLayout.Y_AXIS));
    JLabel info = new JLabel("Antall utveier: ");
    this.antUtveierTekst = new JLabel("0");
    infoPanel.add(info);
    infoPanel.add(antUtveierTekst);
    panel.add(infoPanel, BorderLayout.WEST);
    */

    //Tegner labyrint
    JPanel labPanel = new JPanel();
    labPanel.setLayout(new GridLayout(this.rutenett.length ,this.rutenett[0].length));
    for(Rute[] nrKolonne : rutenett){
      for(Rute r : nrKolonne){
        r.initGUI();
        labPanel.add(r);
      }
    }
    //Legger inn panelene i vindu
    panel.add(labPanel, BorderLayout.CENTER);
    vindu.pack();
    vindu.setVisible(true);
  }

  //Legger til utveier i listen for mulige utveier
  public void leggTilUtVei(ArrayList utvei){
    this.utveier.add(utvei);
  }

  //Nullstiller ArrayList med utveier
  public void nullStill(){

    /*
    for(ArrayList<Rute> veien : this.utveier){
      for(Rute r : veien){
        r.setBackground(Color.WHITE);
      }
      }
    this.utveier.clear();
    */
    for(Thread tra : this.aktiveTrader){
      tra.interrupt();
    }
    this.aktiveTrader.clear();

    for(ArrayList<Rute> veien : this.utveier){
      for(Rute r : veien){
        r.nulstillRute();
      }
      }
    this.utveier.clear();
  }

  //Finner den korteste utveien
  public ArrayList finnKorteste(){
    ArrayList<Rute> korteste = this.utveier.get(0);

    for(ArrayList<Rute> ar : this.utveier){
      if(ar.size() < korteste.size()){
        korteste = ar;
      }
    }
    return korteste;
  }
  /*
  //Farger korteste veien blå og skriver ut antall veier
  public void visVeier(){
    ArrayList<Rute> veien = finnKorteste();
    for(Rute r : veien){
      r.setBackground(Color.BLUE);
    }
    int antallUtveier = this.utveier.size();
    this.antUtveierTekst.setText("ANTALL UTVEIER: " + String.valueOf(antallUtveier));
  }
  */

  //Alternativ
  public void visVeier(){
    this.antUtveierTekst.setText("ANTALL UTVEIER: " + this.utveier.size());
    if(this.utveier.size()==0) return;
    //ArrayList<Rute> veien = finnKorteste();
    //Color[] farger =  {Color.BLUE, Color.RED, Color.ORANGE, Color.YELLOW};
    for(ArrayList<Rute> vei : this.utveier){
      Thread ny = new Thread(new Fargelegger(vei, Color.BLUE));
      aktiveTrader.add(ny);
      ny.start();
    }


  }

  //Lager rutenettet og alle nabone til rutene
  public Rute[][] lagRutenett(File filnavn) throws FileNotFoundException {

    Scanner fil = new Scanner(filnavn);
    String[] maalDeler = fil.nextLine().split(" ");
    int antallRader = Integer.parseInt(maalDeler[0]);
    int antallKolonner = Integer.parseInt(maalDeler[1]);
    Rute[][] nyR = new Rute[antallRader][antallKolonner];

    //Dobbel for løkke for å lagre i 2D liste
    //Velger en rad og tar dermed rute fra venstre til hoyere
    for(int radNr = 0; radNr < antallRader; radNr++){
      String radLinje = fil.nextLine();

      for(int kolonneNr = 0; kolonneNr < antallKolonner; kolonneNr++){
         char c = radLinje.charAt(kolonneNr);
         Rute nyRute;
         if(c=='.'){
           if((radNr==0) || (radNr==antallRader-1) || (kolonneNr==0) || (kolonneNr==antallKolonner-1)){
             nyRute = new Aapning(kolonneNr, radNr, this);
           }
           else{
             nyRute = new HvitRute(kolonneNr, radNr, this);
           }
         }

         else{
           nyRute = new SortRute(kolonneNr, radNr, this);
         }

         nyR[radNr][kolonneNr] = nyRute;
         //System.out.println("Rute kolonne: " + kolonneNr + " rad: " + radNr + " type: (" + nyR[radNr][kolonneNr].tilTegn());
      }
    }

    //Legger til naboene til alle rutene
    for(int y = 0; y < antallRader; y++){
      for(int x = 0; x < antallKolonner; x++){

        //nord
        if((y>0)){nyR[y][x].naboer[0] = nyR[y-1][x];}

        //syd
        if(y<antallRader-1){nyR[y][x].naboer[1] = nyR[y+1][x];}

        //Vest
        if(x>0){nyR[y][x].naboer[2] = nyR[y][x-1];}

        //Ost

        if(x<antallKolonner-1){nyR[y][x].naboer[3] = nyR[y][x+1];}

      }
    }
    return nyR;
    }


    //Fra oblig 6 - brukes ikke
    public ArrayList finnUtveiFra(int x, int y){
      utveier = new ArrayList<ArrayList<Rute>>();
      this.rutenett[y][x].finnUtvei();
      return utveier;
    }

    //Fra oblig 6 - brukes ikke
    public String toString(){
      String utskrift = "";

      //Skriv ut laberint i terminal
      for(int y = 0; y < rutenett.length; y++){
        String linje = "";
        for(int x = 0; x < rutenett[y].length; x++){
          linje += rutenett[y][x].tilTegn();
        }
        utskrift += linje + "\n";
      }
      return utskrift;
    }
}
