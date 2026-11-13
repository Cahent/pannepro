@extends('layout')
@section('title','Anmeldung')
@section('content')<div class="card" style="max-width:480px;margin:50px auto"><h1>Kundenanmeldung</h1><form method="post" action="{{route('login')}}">@csrf<label>E-Mail<input name="email" type="email" value="{{old('email')}}" autocomplete="email" required></label><label>Passwort<input name="password" type="password" autocomplete="current-password" required></label><button type="submit">Anmelden</button></form><p>Neuer Kunde? <a href="{{route('register')}}">Konto erstellen</a></p></div>@endsection
