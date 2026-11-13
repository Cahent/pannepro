<?php
namespace App\Console\Commands;
use App\Models\User;
use Illuminate\Console\Command;
use Illuminate\Support\Facades\Hash;
class CreateAdmin extends Command {
 protected $signature='pannepro:admin {email}';
 protected $description='Erstellt oder aktualisiert einen Administrator mit interaktiv eingegebenem Passwort';
 public function handle():int {
    $email=$this->argument('email');
    if(!filter_var($email,FILTER_VALIDATE_EMAIL)){$this->error('Ungültige E-Mail.');return self::FAILURE;}
    $password=$this->secret('Admin-Passwort (mindestens 12 Zeichen)');
    if(!is_string($password)||strlen($password)<12){$this->error('Passwort zu kurz.');return self::FAILURE;}
    User::updateOrCreate(['email'=>$email],['name'=>'Administrator','role'=>'admin','password'=>Hash::make($password)]);
    $this->info('Admin-Konto angelegt/aktualisiert.');return self::SUCCESS;
 }
}
